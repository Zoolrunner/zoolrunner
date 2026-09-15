/* Darwin's locale-table ABI must survive Mozilla's -fshort-wchar flag. */
#ifdef ZR_STDLIB_FIRST
#include <stdlib.h>
#endif
#include <ctype.h>
#include <stdio.h>
#include <stddef.h>

typedef char short_wchar_required[sizeof(wchar_t) == 2 ? 1 : -1];
typedef char system_rune_required[sizeof(rune_t) == 4 ? 1 : -1];

int main()
{
    for (int c = 0; c < 128; ++c) {
        int upper = c >= 'A' && c <= 'Z';
        int lower = c >= 'a' && c <= 'z';
        int digit = c >= '0' && c <= '9';
        if (tolower(c) != (upper ? c + 'a' - 'A' : c) ||
            toupper(c) != (lower ? c - 'a' + 'A' : c) ||
            !!isalpha(c) != (upper || lower) || !!isdigit(c) != digit) {
            printf("Darwin ctype mismatch at %d\n", c);
            return 1;
        }
    }
    if (tolower(EOF) != EOF || toupper(EOF) != EOF) return 2;
    puts("Darwin ctype and 16-bit wchar_t compatibility passed");
    return 0;
}
