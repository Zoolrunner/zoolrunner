/* Mirror a guest test's output to its report and the emulated console.
 * The original installation CD has no tee. Preserve the child's exit status. */
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>

static int write_all(int fd, const char *bytes, size_t length)
{
    while (length) {
        ssize_t count = write(fd, bytes, length);
        if (count < 0 && errno == EINTR) continue;
        if (count <= 0) return 0;
        bytes += count;
        length -= count;
    }
    return 1;
}

int main(int argc, char **argv)
{
    int output[2], console, status, failed = 0;
    pid_t child;
    char bytes[4096];
    ssize_t count;
    if (argc != 2 || pipe(output)) return 2;
    child = fork();
    if (child < 0) return 2;
    if (!child) {
        close(output[0]);
        if (dup2(output[1], 1) < 0 || dup2(output[1], 2) < 0) _exit(127);
        close(output[1]);
        execl("/bin/sh", "sh", argv[1], (char *)0);
        _exit(127);
    }
    close(output[1]);
    console = open("/dev/console", O_WRONLY | O_NONBLOCK);
    for (;;) {
        count = read(output[0], bytes, sizeof(bytes));
        if (count < 0 && errno == EINTR) continue;
        if (count < 0) failed = 1;
        if (count <= 0) break;
        if (!write_all(1, bytes, count)) failed = 1;
        /* Retain diagnostics if a GUI test hangs and the VM times out.
         * A caller may redirect stdout to a pipe, where fsync is inapplicable. */
        (void)fsync(1);
        if (console >= 0 && !write_all(console, bytes, count)) {
            close(console);
            console = -1;
        }
    }
    close(output[0]);
    if (console >= 0) close(console);
    while (waitpid(child, &status, 0) < 0) {
        if (errno != EINTR) return 2;
    }
    if (failed) return 2;
    if (WIFEXITED(status)) return WEXITSTATUS(status);
    if (WIFSIGNALED(status)) return 128 + WTERMSIG(status);
    return 2;
}
