/* Regression for Gecko's caller-owned input replay after XML_StopParser.
 * A stylesheet can block an XHTML document at </link>, before a long script.
 * Do not consume its CDATA, dispatch later callbacks, or retain bytes which
 * nsExpatDriver will submit again after XML_ResumeParser. */
#include "expat_config.h"
#include "expat.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct State {
    XML_Parser parser;
    int blocked;
    int pauses;
    int ends;
    int late;
};

static int same(const XML_Char *wide, const char *ascii)
{
    while (*wide && *ascii && *wide == (unsigned char)*ascii) {
        ++wide;
        ++ascii;
    }
    return !*wide && !*ascii;
}

static void XMLCALL ended(void *data, const XML_Char *name)
{
    struct State *state = (struct State *)data;
    if (state->blocked) state->late = 1;
    ++state->ends;
    if (same(name, "link") || same(name, "script") || same(name, "root")) {
        state->blocked = 1;
        ++state->pauses;
        if (XML_StopParser(state->parser, XML_TRUE) != XML_STATUS_OK)
            state->late = 1;
    }
}

static void XMLCALL chars(void *data, const XML_Char *text, int length)
{
    struct State *state = (struct State *)data;
    (void)text;
    (void)length;
    if (state->blocked) state->late = 1;
}

static int check(const char *input, int length, int chunk, int pauses, int ends)
{
    struct State state;
    int offset = 0, finished = 0;
    memset(&state, 0, sizeof(state));
    state.parser = XML_ParserCreate(NULL);
    if (!state.parser) return 0;
    XML_SetUserData(state.parser, &state);
    XML_SetEndElementHandler(state.parser, ended);
    XML_SetCharacterDataHandler(state.parser, chars);
    while (!finished) {
        int count = length - offset;
        int final;
        enum XML_Status status;
        if (count > chunk) count = chunk;
        final = offset + count == length;
        status = XML_Parse(state.parser, input + offset, count, final);
        if (state.blocked) {
            XML_Index consumed = XML_GetCurrentByteIndex(state.parser);
            if (status != XML_STATUS_ERROR ||
                XML_GetErrorCode(state.parser) != XML_ERROR_SUSPENDED ||
                consumed < 0 || consumed > offset + count || state.late) {
                fprintf(stderr, "Blocking failure: status=%d error=%d offset=%d consumed=%ld late=%d\n",
                        status, XML_GetErrorCode(state.parser), offset,
                        (long)consumed, state.late);
                XML_ParserFree(state.parser);
                return 0;
            }
            offset = (int)consumed;
            state.blocked = 0;
            if (XML_ResumeParser(state.parser) != XML_STATUS_OK) return 0;
        } else {
            if (status != XML_STATUS_OK) {
                fprintf(stderr, "Replay failure: error=%d offset=%d\n",
                        XML_GetErrorCode(state.parser), offset);
                XML_ParserFree(state.parser);
                return 0;
            }
            offset += count;
            finished = final;
        }
    }
    XML_ParserFree(state.parser);
    return state.pauses == pauses && state.ends == ends && !state.late;
}

int main(void)
{
    char ascii[8192], encoded[16384];
    const int chunks[] = { 7, 16, 31, 4096, 16384 };
    int kind, mode, i, n, length, checks = 0;
    strcpy(ascii, "<root><link/><script><![CDATA[");
    n = (int)strlen(ascii);
    memset(ascii + n, 'x', 6000);
    strcpy(ascii + n + 6000, "]]></script><tail/></root>\n<!--epilog-->");
    for (kind = 0; kind < 2; ++kind) {
        if (kind) strcpy(ascii, "<root/>\n<!--epilog-->");
        n = (int)strlen(ascii);
        for (mode = 0; mode < 3; ++mode) {
            length = 0;
            if (!mode) {
                memcpy(encoded, ascii, n);
                length = n;
            } else {
                encoded[length++] = mode == 1 ? (char)0xff : (char)0xfe;
                encoded[length++] = mode == 1 ? (char)0xfe : (char)0xff;
                for (i = 0; i < n; ++i) {
                    encoded[length++] = mode == 1 ? ascii[i] : 0;
                    encoded[length++] = mode == 1 ? 0 : ascii[i];
                }
            }
            for (i = 0; i < (int)(sizeof(chunks) / sizeof(chunks[0])); ++i) {
                if (!check(encoded, length, chunks[i], kind ? 1 : 3, kind ? 1 : 4)) {
                    fprintf(stderr, "Expat blocking failed: encoding=%d chunk=%d\n", mode, chunks[i]);
                    return 1;
                }
                ++checks;
            }
        }
    }
    printf("Expat stylesheet/script blocking and input replay: %d checks passed\n", checks);
    return 0;
}
