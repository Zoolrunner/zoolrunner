/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set sw=4 ts=8 et tw=78:
 *
 * ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is Mozilla Communicator client code, released
 * March 31, 1998.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either of the GNU General Public License Version 2 or later (the "GPL"),
 * or the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

/*
 * JS regular expressions, after Perl.
 */
#include "jsstddef.h"
#include <stdlib.h>
#include <string.h>
#include "jstypes.h"
#include "jsarena.h" /* Added by JSIFY */
#include "jsutil.h" /* Added by JSIFY */
#include "jsapi.h"
#include "jsarray.h"
#include "jsatom.h"
#include "jsbool.h"
#include "jscntxt.h"
#include "jsconfig.h"
#include "jsfun.h"
#include "jsgc.h"
#include "jsinterp.h"
#include "jsiteres6.h"
#include "jslock.h"
#include "jsnum.h"
#include "jsobj.h"
#include "jsopcode.h"
#include "jsprf.h"
#include "jsregexp.h"
#include "jsrealm.h"
#include "jsscan.h"
#include "jsstr.h"
#include "jssymbol.h"

JSBool
js_IsRegExp(JSContext *cx, jsval value, JSBool *result)
{
    jsval roots[2];
    JSTempValueRooter root;
    jsid id;
    JSBool ok;

    *result = JS_FALSE;
    if (JSVAL_IS_PRIMITIVE(value))
        return JS_TRUE;
    roots[0] = value;
    roots[1] = JSVAL_VOID;
    JS_PUSH_TEMP_ROOT(cx, 2, roots, &root);
    ok = js_WellKnownSymbolId(cx, JS_WKS_MATCH, &id) &&
         OBJ_GET_PROPERTY(cx, JSVAL_TO_OBJECT(value), id, &roots[1]);
    if (ok) {
        if (JSVAL_IS_VOID(roots[1]))
            *result = JSVAL_IS_REGEXP(cx, value);
        else
            ok = js_ValueToBoolean(cx, roots[1], result);
    }
    JS_POP_TEMP_ROOT(cx, &root);
    return ok;
}

/* Note : contiguity of 'simple opcodes' is important for SimpleMatch() */
typedef enum REOp {
    REOP_EMPTY         = 0,  /* match rest of input against rest of r.e. */
    REOP_ALT           = 1,  /* alternative subexpressions in kid and next */
    REOP_SIMPLE_START  = 2,  /* start of 'simple opcodes' */
    REOP_BOL           = 2,  /* beginning of input (or line if multiline) */
    REOP_EOL           = 3,  /* end of input (or line if multiline) */
    REOP_WBDRY         = 4,  /* match "" at word boundary */
    REOP_WNONBDRY      = 5,  /* match "" at word non-boundary */
    REOP_DOT           = 6,  /* stands for any character */
    REOP_DIGIT         = 7,  /* match a digit char: [0-9] */
    REOP_NONDIGIT      = 8,  /* match a non-digit char: [^0-9] */
    REOP_ALNUM         = 9,  /* match an alphanumeric char: [0-9a-z_A-Z] */
    REOP_NONALNUM      = 10, /* match a non-alphanumeric char: [^0-9a-z_A-Z] */
    REOP_SPACE         = 11, /* match a whitespace char */
    REOP_NONSPACE      = 12, /* match a non-whitespace char */
    REOP_BACKREF       = 13, /* back-reference (e.g., \1) to a parenthetical */
    REOP_FLAT          = 14, /* match a flat string */
    REOP_FLAT1         = 15, /* match a single char */
    REOP_FLATi         = 16, /* case-independent REOP_FLAT */
    REOP_FLAT1i        = 17, /* case-independent REOP_FLAT1 */
    REOP_UCFLAT1       = 18, /* single Unicode char */
    REOP_UCFLAT1i      = 19, /* case-independent REOP_UCFLAT1 */
    REOP_UCFLAT        = 20, /* flat Unicode string; len immediate counts chars */
    REOP_UCFLATi       = 21, /* case-independent REOP_UCFLAT */
    REOP_CLASS         = 22, /* character class with index */
    REOP_NCLASS        = 23, /* negated character class with index */
    REOP_UCFLAT32      = 24, /* Unicode code point, four-byte operand */
    REOP_SIMPLE_END    = 24, /* end of 'simple opcodes' */
    REOP_QUANT         = 25, /* quantified atom: atom{1,2} */
    REOP_STAR          = 26, /* zero or more occurrences of kid */
    REOP_PLUS          = 27, /* one or more occurrences of kid */
    REOP_OPT           = 28, /* optional subexpression in kid */
    REOP_LPAREN        = 29, /* left paren bytecode: kid is u.num'th sub-regexp */
    REOP_RPAREN        = 30, /* right paren bytecode */
    REOP_JUMP          = 31, /* for deoptimized closure loops */
    REOP_DOTSTAR       = 32, /* optimize .* to use a single opcode */
    REOP_ANCHOR        = 33, /* like .* but skips left context to unanchored r.e. */
    REOP_EOLONLY       = 34, /* $ not preceded by any pattern */
    REOP_BACKREFi      = 37, /* case-independent REOP_BACKREF */
    REOP_LPARENNON     = 41, /* non-capturing version of REOP_LPAREN */
    REOP_ASSERT        = 43, /* zero width positive lookahead assertion */
    REOP_ASSERT_NOT    = 44, /* zero width negative lookahead assertion */
    REOP_ASSERTTEST    = 45, /* sentinel at end of assertion child */
    REOP_ASSERTNOTTEST = 46, /* sentinel at end of !assertion child */
    REOP_MINIMALSTAR   = 47, /* non-greedy version of * */
    REOP_MINIMALPLUS   = 48, /* non-greedy version of + */
    REOP_MINIMALOPT    = 49, /* non-greedy version of ? */
    REOP_MINIMALQUANT  = 50, /* non-greedy version of {} */
    REOP_ENDCHILD      = 51, /* sentinel at end of quantifier child */
    REOP_REPEAT        = 52, /* directs execution of greedy quantifier */
    REOP_MINIMALREPEAT = 53, /* directs execution of non-greedy quantifier */
    REOP_ALTPREREQ     = 54, /* prerequisite for ALT, either of two chars */
    REOP_ALTPREREQ2    = 55, /* prerequisite for ALT, a char or a class */
    REOP_ENDALT        = 56, /* end of final alternate */
    REOP_CONCAT        = 57, /* concatenation of terms (parse time only) */

    REOP_END
} REOp;

#define REOP_IS_SIMPLE(op) ((unsigned)((op)-REOP_SIMPLE_START) <= \
                            (unsigned)(REOP_SIMPLE_END-REOP_SIMPLE_START))

struct RENode {
    REOp            op;         /* r.e. op bytecode */
    RENode          *next;      /* next in concatenation order */
    void            *kid;       /* first operand */
    union {
        void        *kid2;      /* second operand */
        jsint       num;        /* could be a number */
        size_t      parenIndex; /* or a parenthesis index */
        struct {                /* or a quantifier range */
            uintN  min;
            uintN  max;
            JSPackedBool greedy;
        } range;
        struct {                /* or a character class */
            size_t  startIndex;
            size_t  kidlen;     /* length of string at kid, in jschars */
            size_t  index;      /* index into class list */
            uint32  bmsize;     /* bitmap size, based on max char code */
            JSPackedBool sense;
        } ucclass;
        struct {                /* or a literal sequence */
            uint32  chr;        /* of one character */
            size_t  length;     /* or many (via the kid) */
        } flat;
        struct {
            RENode  *kid2;      /* second operand from ALT */
            jschar  ch1;        /* match char for ALTPREREQ */
            jschar  ch2;        /* ditto, or class index for ALTPREREQ2 */
        } altprereq;
    } u;
};

#define RE_IS_LETTER(c)     (((c >= 'A') && (c <= 'Z')) ||                    \
                             ((c >= 'a') && (c <= 'z')) )
#define RE_IS_LINE_TERM(c)  ((c == '\n') || (c == '\r') ||                    \
                             (c == LINE_SEPARATOR) || (c == PARA_SEPARATOR))

#define CLASS_CACHE_SIZE    4

typedef struct CompilerState {
    JSBool modern;                 /* grammar edition, independent of caller */
    JSContext       *context;
    JSTokenStream   *tokenStream; /* For reporting errors */
    const jschar    *cpbegin;
    const jschar    *cpend;
    const jschar    *cp;
    size_t          parenCount;
    size_t          classCount;   /* number of [] encountered */
    size_t          treeDepth;    /* maximum depth of parse tree */
    size_t          progLength;   /* estimated bytecode length */
    RENode          *result;
    size_t          classBitmapsMem; /* memory to hold all class bitmaps */
    struct {
        const jschar *start;        /* small cache of class strings */
        size_t length;              /* since they're often the same */
        size_t index;
    } classCache[CLASS_CACHE_SIZE];
    uint16          flags;
} CompilerState;

typedef struct EmitStateStackEntry {
    jsbytecode      *altHead;       /* start of REOP_ALT* opcode */
    jsbytecode      *nextAltFixup;  /* fixup pointer to next-alt offset */
    jsbytecode      *nextTermFixup; /* fixup ptr. to REOP_JUMP offset */
    jsbytecode      *endTermFixup;  /* fixup ptr. to REOPT_ALTPREREQ* offset */
    RENode          *continueNode;  /* original REOP_ALT* node being stacked */
    jsbytecode      continueOp;     /* REOP_JUMP or REOP_ENDALT continuation */
    JSPackedBool    jumpToJumpFlag; /* true if we've patched jump-to-jump to
                                       avoid 16-bit unsigned offset overflow */
} EmitStateStackEntry;

/*
 * Immediate operand sizes and getter/setters.  Unlike the ones in jsopcode.h,
 * the getters and setters take the pc of the offset, not of the opcode before
 * the offset.
 */
#define ARG_LEN             2
#define GET_ARG(pc)         ((uint16)(((pc)[0] << 8) | (pc)[1]))
#define SET_ARG(pc, arg)    ((pc)[0] = (jsbytecode) ((arg) >> 8),       \
                             (pc)[1] = (jsbytecode) (arg))

#define OFFSET_LEN          ARG_LEN
#define OFFSET_MAX          (JS_BIT(ARG_LEN * 8) - 1)
#define GET_OFFSET(pc)      GET_ARG(pc)

/*
 * Maximum supported tree depth is maximum size of EmitStateStackEntry stack.
 * For sanity, we limit it to 2^24 bytes.
 */
#define TREE_DEPTH_MAX  (JS_BIT(24) / sizeof(EmitStateStackEntry))

/*
 * The maximum memory that can be allocated for class bitmaps.
 * For sanity, we limit it to 2^24 bytes.
 */
#define CLASS_BITMAPS_MEM_LIMIT JS_BIT(24)

/*
 * Functions to get size and write/read bytecode that represent small indexes
 * compactly.
 * Each byte in the code represent 7-bit chunk of the index. 8th bit when set
 * indicates that the following byte brings more bits to the index. Otherwise
 * this is the last byte in the index bytecode representing highest index bits.
 */
static size_t
GetCompactIndexWidth(size_t index)
{
    size_t width;

    for (width = 1; (index >>= 7) != 0; ++width) { }
    return width;
}

static jsbytecode *
WriteCompactIndex(jsbytecode *pc, size_t index)
{
    size_t next;

    while ((next = index >> 7) != 0) {
        *pc++ = (jsbytecode)(index | 0x80);
        index = next;
    }
    *pc++ = (jsbytecode)index;
    return pc;
}

static jsbytecode *
ReadCompactIndex(jsbytecode *pc, size_t *result)
{
    size_t nextByte;

    nextByte = *pc++;
    if ((nextByte & 0x80) == 0) {
        /*
         * Short-circuit the most common case when compact index <= 127.
         */
        *result = nextByte;
    } else {
        size_t shift = 7;
        *result = 0x7F & nextByte;
        do {
            nextByte = *pc++;
            *result |= (nextByte & 0x7F) << shift;
            shift += 7;
        } while ((nextByte & 0x80) != 0);
    }
    return pc;
}

typedef struct RECapture {
    ptrdiff_t index;           /* start of contents, -1 for empty  */
    size_t length;             /* length of capture */
} RECapture;

typedef struct REMatchState {
    const jschar *cp;
    RECapture parens[1];      /* first of 're->parenCount' captures,
                                 allocated at end of this struct */
} REMatchState;

struct REBackTrackData;

typedef struct REProgState {
    jsbytecode *continue_pc;        /* current continuation data */
    jsbytecode continue_op;
    ptrdiff_t index;                /* progress in text */
    size_t parenSoFar;              /* highest indexed paren started */
    union {
        struct {
            uintN min;             /* current quantifier limits */
            uintN max;
        } quantifier;
        struct {
            size_t top;             /* backtrack stack state */
            size_t sz;
        } assertion;
    } u;
} REProgState;

typedef struct REBackTrackData {
    size_t sz;                      /* size of previous stack entry */
    jsbytecode *backtrack_pc;       /* where to backtrack to */
    jsbytecode backtrack_op;
    const jschar *cp;               /* index in text of match at backtrack */
    size_t parenIndex;              /* start index of saved paren contents */
    size_t parenCount;              /* # of saved paren contents */
    size_t saveStateStackTop;       /* number of parent states */
    /* saved parent states follow */
    /* saved paren contents follow */
} REBackTrackData;

#define INITIAL_STATESTACK  100
#define INITIAL_BACKTRACK   8000

typedef struct REGlobalData {
    JSContext *cx;
    JSRegExp *regexp;               /* the RE in execution */
    JSBool ok;                      /* runtime error (out_of_memory only?) */
    JSBool sticky;                  /* match only at the requested offset */
    size_t start;                   /* offset to start at */
    ptrdiff_t skipped;              /* chars skipped anchoring this r.e. */
    const jschar    *cpbegin;       /* text base address */
    const jschar    *cpend;         /* text limit address */

    REProgState *stateStack;        /* stack of state of current parents */
    size_t stateStackTop;
    size_t stateStackLimit;

    REBackTrackData *backTrackStack;/* stack of matched-so-far positions */
    REBackTrackData *backTrackSP;
    size_t backTrackStackSize;
    size_t cursz;                   /* size of current stack entry */

    JSArenaPool     pool;           /* It's faster to use one malloc'd pool
                                       than to malloc/free the three items
                                       that are allocated from this pool */
} REGlobalData;

/*
 * 1. If IgnoreCase is false, return ch.
 * 2. Let u be ch converted to upper case as if by calling
 *    String.prototype.toUpperCase on the one-character string ch.
 * 3. If u does not consist of a single character, return ch.
 * 4. Let cu be u's character.
 * 5. If ch's code point value is greater than or equal to decimal 128 and cu's
 *    code point value is less than decimal 128, then return ch.
 * 6. Return cu.
 */
static jschar
upcase(jschar ch)
{
    jschar cu = JS_TOUPPER(ch);
    if (ch >= 128 && cu < 128)
        return ch;
    return cu;
}

static jschar
downcase(jschar ch)
{
    jschar cl = JS_TOLOWER(ch);
    if (cl >= 128 && ch < 128)
        return ch;
    return cl;
}

/* Construct and initialize an RENode, returning NULL for out-of-memory */
static RENode *
NewRENode(CompilerState *state, REOp op)
{
    JSContext *cx;
    RENode *ren;

    cx = state->context;
    JS_ARENA_ALLOCATE_CAST(ren, RENode *, &cx->tempPool, sizeof *ren);
    if (!ren) {
        JS_ReportOutOfMemory(cx);
        return NULL;
    }
    ren->op = op;
    ren->next = NULL;
    ren->kid = NULL;
    return ren;
}

/*
 * Validates and converts hex ascii value.
 */
static JSBool
isASCIIHexDigit(jschar c, uintN *digit)
{
    uintN cv = c;

    if (cv < '0')
        return JS_FALSE;
    if (cv <= '9') {
        *digit = cv - '0';
        return JS_TRUE;
    }
    cv |= 0x20;
    if (cv >= 'a' && cv <= 'f') {
        *digit = cv - 'a' + 10;
        return JS_TRUE;
    }
    return JS_FALSE;
}


/* Leave the cursor unchanged when Annex B must use IdentityEscape. */
static JSBool
CompleteHexEscape(const jschar **position, const jschar *end, uintN digits,
                  uintN *value)
{
    const jschar *cp = *position;
    uintN i, digit, n = 0;
    for (i = 0; i < digits; ++i) {
        if (cp == end || !isASCIIHexDigit(*cp++, &digit)) return JS_FALSE;
        n = (n << 4) | digit;
    }
    *position = cp;
    *value = n;
    return JS_TRUE;
}


typedef struct {
    REOp op;
    const jschar *errPos;
    size_t parenIndex;
} REOpData;


/*
 * Process the op against the two top operands, reducing them to a single
 * operand in the penultimate slot. Update progLength and treeDepth.
 */
static JSBool
ProcessOp(CompilerState *state, REOpData *opData, RENode **operandStack,
          intN operandSP)
{
    RENode *result;

    switch (opData->op) {
    case REOP_ALT:
        result = NewRENode(state, REOP_ALT);
        if (!result)
            return JS_FALSE;
        result->kid = operandStack[operandSP - 2];
        result->u.kid2 = operandStack[operandSP - 1];
        operandStack[operandSP - 2] = result;

        if (state->treeDepth == TREE_DEPTH_MAX) {
            js_ReportCompileErrorNumber(state->context, state->tokenStream,
                                        JSREPORT_TS | JSREPORT_ERROR,
                                        JSMSG_REGEXP_TOO_COMPLEX);
            return JS_FALSE;
        }
        ++state->treeDepth;

        /*
         * Look at both alternates to see if there's a FLAT or a CLASS at
         * the start of each. If so, use a prerequisite match.
         */
        if (((RENode *) result->kid)->op == REOP_FLAT &&
            ((RENode *) result->u.kid2)->op == REOP_FLAT &&
            (state->flags & (JSREG_FOLD | JSREG_UNICODE)) == 0) {
            result->op = REOP_ALTPREREQ;
            result->u.altprereq.ch1 = ((RENode *) result->kid)->u.flat.chr;
            result->u.altprereq.ch2 = ((RENode *) result->u.kid2)->u.flat.chr;
            /* ALTPREREQ, <end>, uch1, uch2, <next>, ...,
                                            JUMP, <end> ... ENDALT */
            state->progLength += 13;
        }
        else
        if (((RENode *) result->kid)->op == REOP_CLASS &&
            ((RENode *) result->kid)->u.ucclass.index < 256 &&
            ((RENode *) result->u.kid2)->op == REOP_FLAT &&
            (state->flags & (JSREG_FOLD | JSREG_UNICODE)) == 0) {
            result->op = REOP_ALTPREREQ2;
            result->u.altprereq.ch1 = ((RENode *) result->u.kid2)->u.flat.chr;
            result->u.altprereq.ch2 = ((RENode *) result->kid)->u.ucclass.index;
            /* ALTPREREQ2, <end>, uch1, uch2, <next>, ...,
                                            JUMP, <end> ... ENDALT */
            state->progLength += 13;
        }
        else
        if (((RENode *) result->kid)->op == REOP_FLAT &&
            ((RENode *) result->u.kid2)->op == REOP_CLASS &&
            ((RENode *) result->u.kid2)->u.ucclass.index < 256 &&
            (state->flags & (JSREG_FOLD | JSREG_UNICODE)) == 0) {
            result->op = REOP_ALTPREREQ2;
            result->u.altprereq.ch1 = ((RENode *) result->kid)->u.flat.chr;
            result->u.altprereq.ch2 =
                ((RENode *) result->u.kid2)->u.ucclass.index;
            /* ALTPREREQ2, <end>, uch1, uch2, <next>, ...,
                                          JUMP, <end> ... ENDALT */
            state->progLength += 13;
        }
        else {
            /* ALT, <next>, ..., JUMP, <end> ... ENDALT */
            state->progLength += 7;
        }
        break;

    case REOP_CONCAT:
        result = operandStack[operandSP - 2];
        while (result->next)
            result = result->next;
        result->next = operandStack[operandSP - 1];
        break;

    case REOP_ASSERT:
    case REOP_ASSERT_NOT:
    case REOP_LPARENNON:
    case REOP_LPAREN:
        /* These should have been processed by a close paren. */
        js_ReportCompileErrorNumberUC(state->context, state->tokenStream,
                                      JSREPORT_TS | JSREPORT_ERROR,
                                      JSMSG_MISSING_PAREN, opData->errPos);
        return JS_FALSE;

    default:;
    }
    return JS_TRUE;
}

/*
 * Parser forward declarations.
 */
static JSBool ParseTerm(CompilerState *state);
static JSBool ParseQuantifier(CompilerState *state);
static intN ParseMinMaxQuantifier(CompilerState *state, JSBool ignoreValues);

/*
 * Top-down regular expression grammar, based closely on Perl4.
 *
 *  regexp:     altern                  A regular expression is one or more
 *              altern '|' regexp       alternatives separated by vertical bar.
 */
#define INITIAL_STACK_SIZE  128

static JSBool
ParseRegExp(CompilerState *state)
{
    size_t parenIndex;
    RENode *operand;
    REOpData *operatorStack;
    RENode **operandStack;
    REOp op;
    intN i;
    JSBool result = JS_FALSE;

    intN operatorSP = 0, operatorStackSize = INITIAL_STACK_SIZE;
    intN operandSP = 0, operandStackSize = INITIAL_STACK_SIZE;

    /* Watch out for empty regexp */
    if (state->cp == state->cpend) {
        state->result = NewRENode(state, REOP_EMPTY);
        return (state->result != NULL);
    }

    operatorStack = (REOpData *)
        JS_malloc(state->context, sizeof(REOpData) * operatorStackSize);
    if (!operatorStack)
        return JS_FALSE;

    operandStack = (RENode **)
        JS_malloc(state->context, sizeof(RENode *) * operandStackSize);
    if (!operandStack)
        goto out;

    for (;;) {
        parenIndex = state->parenCount;
        if (state->cp == state->cpend) {
            /*
             * If we are at the end of the regexp and we're short one or more
             * operands, the regexp must have the form /x|/ or some such, with
             * left parentheses making us short more than one operand.
             */
            if (operatorSP >= operandSP) {
                operand = NewRENode(state, REOP_EMPTY);
                if (!operand)
                    goto out;
                goto pushOperand;
            }
        } else {
            switch (*state->cp) {
            case '(':
                ++state->cp;
                if (state->cp + 1 < state->cpend &&
                    *state->cp == '?' &&
                    (state->cp[1] == '=' ||
                     state->cp[1] == '!' ||
                     state->cp[1] == ':')) {
                    switch (state->cp[1]) {
                    case '=':
                        op = REOP_ASSERT;
                        /* ASSERT, <next>, ... ASSERTTEST */
                        state->progLength += 4;
                        break;
                    case '!':
                        op = REOP_ASSERT_NOT;
                        /* ASSERTNOT, <next>, ... ASSERTNOTTEST */
                        state->progLength += 4;
                        break;
                    default:
                        op = REOP_LPARENNON;
                        break;
                    }
                    state->cp += 2;
                } else {
                    op = REOP_LPAREN;
                    /* LPAREN, <index>, ... RPAREN, <index> */
                    state->progLength
                        += 2 * (1 + GetCompactIndexWidth(parenIndex));
                    state->parenCount++;
                    if (state->parenCount == 65535) {
                        js_ReportCompileErrorNumber(state->context,
                                                    state->tokenStream,
                                                    JSREPORT_TS |
                                                    JSREPORT_ERROR,
                                                    JSMSG_TOO_MANY_PARENS);
                        goto out;
                    }
                }
                goto pushOperator;

            case ')':
                /*
                 * If there's no stacked open parenthesis, throw syntax error.
                 */
                for (i = operatorSP - 1; ; i--) {
                    if (i < 0) {
                        js_ReportCompileErrorNumber(state->context,
                                                    state->tokenStream,
                                                    JSREPORT_TS |
                                                    JSREPORT_ERROR,
                                                    JSMSG_UNMATCHED_RIGHT_PAREN);
                        goto out;
                    }
                    if (operatorStack[i].op == REOP_ASSERT ||
                        operatorStack[i].op == REOP_ASSERT_NOT ||
                        operatorStack[i].op == REOP_LPARENNON ||
                        operatorStack[i].op == REOP_LPAREN) {
                        break;
                    }
                }
                /* FALL THROUGH */

            case '|':
                /* Expected an operand before these, so make an empty one */
                operand = NewRENode(state, REOP_EMPTY);
                if (!operand)
                    goto out;
                goto pushOperand;

            default:
                if (!ParseTerm(state))
                    goto out;
                operand = state->result;
pushOperand:
                if (operandSP == operandStackSize) {
                    operandStackSize += operandStackSize;
                    operandStack = (RENode **)
                        JS_realloc(state->context, operandStack,
                                   sizeof(RENode *) * operandStackSize);
                    if (!operandStack)
                        goto out;
                }
                operandStack[operandSP++] = operand;
                break;
            }
        }

        /* At the end; process remaining operators. */
restartOperator:
        if (state->cp == state->cpend) {
            while (operatorSP) {
                --operatorSP;
                if (!ProcessOp(state, &operatorStack[operatorSP],
                               operandStack, operandSP))
                    goto out;
                --operandSP;
            }
            JS_ASSERT(operandSP == 1);
            state->result = operandStack[0];
            result = JS_TRUE;
            goto out;
        }

        switch (*state->cp) {
        case '|':
            /* Process any stacked 'concat' operators */
            ++state->cp;
            while (operatorSP &&
                   operatorStack[operatorSP - 1].op == REOP_CONCAT) {
                --operatorSP;
                if (!ProcessOp(state, &operatorStack[operatorSP],
                               operandStack, operandSP)) {
                    goto out;
                }
                --operandSP;
            }
            op = REOP_ALT;
            goto pushOperator;

        case ')':
            /*
             * If there's no stacked open parenthesis, throw syntax error.
             */
            for (i = operatorSP - 1; ; i--) {
                if (i < 0) {
                    js_ReportCompileErrorNumber(state->context,
                                                state->tokenStream,
                                                JSREPORT_TS | JSREPORT_ERROR,
                                                JSMSG_UNMATCHED_RIGHT_PAREN);
                    goto out;
                }
                if (operatorStack[i].op == REOP_ASSERT ||
                    operatorStack[i].op == REOP_ASSERT_NOT ||
                    operatorStack[i].op == REOP_LPARENNON ||
                    operatorStack[i].op == REOP_LPAREN) {
                    break;
                }
            }
            ++state->cp;

            /* Process everything on the stack until the open parenthesis. */
            for (;;) {
                JS_ASSERT(operatorSP);
                --operatorSP;
                switch (operatorStack[operatorSP].op) {
                case REOP_ASSERT:
                case REOP_ASSERT_NOT:
                case REOP_LPAREN:
                    operand = NewRENode(state, operatorStack[operatorSP].op);
                    if (!operand)
                        goto out;
                    operand->u.parenIndex =
                        operatorStack[operatorSP].parenIndex;
                    JS_ASSERT(operandSP);
                    operand->kid = operandStack[operandSP - 1];
                    operandStack[operandSP - 1] = operand;
                    if (state->treeDepth == TREE_DEPTH_MAX) {
                        js_ReportCompileErrorNumber(state->context,
                                                    state->tokenStream,
                                                    JSREPORT_TS |
                                                    JSREPORT_ERROR,
                                                    JSMSG_REGEXP_TOO_COMPLEX);
                        goto out;
                    }
                    ++state->treeDepth;
                    /* FALL THROUGH */

                case REOP_LPARENNON:
                    state->result = operandStack[operandSP - 1];
                    if (!ParseQuantifier(state))
                        goto out;
                    operandStack[operandSP - 1] = state->result;
                    goto restartOperator;
                default:
                    if (!ProcessOp(state, &operatorStack[operatorSP],
                                   operandStack, operandSP))
                        goto out;
                    --operandSP;
                    break;
                }
            }
            break;

        case '{':
        {
            const jschar *errp = state->cp;

            if (ParseMinMaxQuantifier(state, JS_TRUE) < 0) {
                /*
                 * This didn't even scan correctly as a quantifier, so we should
                 * treat it as flat.
                 */
                op = REOP_CONCAT;
                goto pushOperator;
            }

            state->cp = errp;
            /* FALL THROUGH */
        }

        case '+':
        case '*':
        case '?':
            js_ReportCompileErrorNumberUC(state->context, state->tokenStream,
                                          JSREPORT_TS | JSREPORT_ERROR,
                                          JSMSG_BAD_QUANTIFIER, state->cp);
            result = JS_FALSE;
            goto out;

        default:
            /* Anything else is the start of the next term. */
            op = REOP_CONCAT;
pushOperator:
            if (operatorSP == operatorStackSize) {
                operatorStackSize += operatorStackSize;
                operatorStack = (REOpData *)
                    JS_realloc(state->context, operatorStack,
                               sizeof(REOpData) * operatorStackSize);
                if (!operatorStack)
                    goto out;
            }
            operatorStack[operatorSP].op = op;
            operatorStack[operatorSP].errPos = state->cp;
            operatorStack[operatorSP++].parenIndex = parenIndex;
            break;
        }
    }
out:
    if (operatorStack)
        JS_free(state->context, operatorStack);
    if (operandStack)
        JS_free(state->context, operandStack);
    return result;
}

/*
 * Hack two bits in CompilerState.flags, for use within FindParenCount to flag
 * its being on the stack, and to propagate errors to its callers.
 */
#define JSREG_FIND_PAREN_COUNT  0x8000
#define JSREG_FIND_PAREN_ERROR  0x4000

/*
 * Magic return value from FindParenCount and GetDecimalValue, to indicate
 * overflow beyond GetDecimalValue's max parameter, or a computed maximum if
 * its findMax parameter is non-null.
 */
#define OVERFLOW_VALUE          ((uintN)-1)

static uintN
FindParenCount(CompilerState *state)
{
    CompilerState temp;
    int i;

    if (state->flags & JSREG_FIND_PAREN_COUNT)
        return OVERFLOW_VALUE;

    /*
     * Copy state into temp, flag it so we never report an invalid backref,
     * and reset its members to parse the entire regexp.  This is obviously
     * suboptimal, but GetDecimalValue calls us only if a backref appears to
     * refer to a forward parenthetical, which is rare.
     */
    temp = *state;
    temp.flags |= JSREG_FIND_PAREN_COUNT;
    temp.cp = temp.cpbegin;
    temp.parenCount = 0;
    temp.classCount = 0;
    temp.progLength = 0;
    temp.treeDepth = 0;
    temp.classBitmapsMem = 0;
    for (i = 0; i < CLASS_CACHE_SIZE; i++)
        temp.classCache[i].start = NULL;

    if (!ParseRegExp(&temp)) {
        state->flags |= JSREG_FIND_PAREN_ERROR;
        return OVERFLOW_VALUE;
    }
    return temp.parenCount;
}

/*
 * Extract and return a decimal value at state->cp.  The initial character c
 * has already been read.  Return OVERFLOW_VALUE if the result exceeds max.
 * Callers who pass a non-null findMax should test JSREG_FIND_PAREN_ERROR in
 * state->flags to discover whether an error occurred under findMax.
 */
static uintN
GetDecimalValue(jschar c, uintN max, uintN (*findMax)(CompilerState *state),
                CompilerState *state)
{
    uintN value = JS7_UNDEC(c);
    JSBool overflow = (value > max && (!findMax || value > findMax(state)));

    /* The following restriction allows simpler overflow checks. */
    JS_ASSERT(max <= ((uintN)-1 - 9) / 10);
    while (state->cp < state->cpend) {
        c = *state->cp;
        if (!JS7_ISDEC(c))
            break;
        value = 10 * value + JS7_UNDEC(c);
        if (!overflow && value > max && (!findMax || value > findMax(state)))
            overflow = JS_TRUE;
        ++state->cp;
    }
    return overflow ? OVERFLOW_VALUE : value;
}

#include "jsregexp-casefold.h"

uint32
js_UnicodeSimpleFold(uint32 point)
{
    size_t low = 0, high = sizeof(regexpCaseFolds) / sizeof(regexpCaseFolds[0]), mid;
    while (low < high) {
        mid = low + (high-low)/2;
        if (point < regexpCaseFolds[mid].from) high = mid;
        else if (point > regexpCaseFolds[mid].from) low = mid+1;
        else return regexpCaseFolds[mid].to;
    }
    return point;
}

static uint32
RegExpPoint(const jschar *cp, const jschar *end, JSBool unicode, uintN *width)
{
    uint32 c;
    *width = 1;
    if (cp >= end) return 0;
    c = *cp;
    if (unicode && c >= 0xD800 && c <= 0xDBFF && cp+1 < end &&
        cp[1] >= 0xDC00 && cp[1] <= 0xDFFF) {
        *width = 2;
        c = 0x10000 + ((c-0xD800)<<10) + cp[1]-0xDC00;
    }
    return c;
}

static JSBool
UnicodeRegExpError(CompilerState *state)
{
    js_ReportCompileErrorNumber(state->context, state->tokenStream,
                               JSREPORT_TS | JSREPORT_ERROR, JSMSG_SYNTAX_ERROR);
    return JS_FALSE;
}

static JSBool
UnicodeRegExpEscape(CompilerState *state, const jschar **position,
                    const jschar *end, uint32 escape, uint32 *value)
{
    const jschar *cp = *position, *saved;
    uint32 n = 0, low;
    uintN count, i, digit;
    switch (escape) {
      case 'f': *value = 12; return JS_TRUE;
      case 'n': *value = 10; return JS_TRUE;
      case 'r': *value = 13; return JS_TRUE;
      case 't': *value = 9; return JS_TRUE;
      case 'v': *value = 11; return JS_TRUE;
      case 'c':
        if (cp == end || !RE_IS_LETTER(*cp)) return UnicodeRegExpError(state);
        *value = *cp++ & 31; break;
      case '0':
        if (cp < end && JS7_ISDEC(*cp)) return UnicodeRegExpError(state);
        *value = 0; break;
      case 'x':
      case 'u':
        if (escape == 'u' && cp < end && *cp == '{') {
            ++cp; count = 0;
            while (cp < end && *cp != '}') {
                if (!isASCIIHexDigit(*cp++, &digit) || n > (0x10FFFF-digit)/16)
                    return UnicodeRegExpError(state);
                n = n*16 + digit; ++count;
            }
            if (!count || cp == end) return UnicodeRegExpError(state);
            ++cp;
        } else {
            count = escape == 'u' ? 4 : 2;
            for (i=0; i<count; ++i) {
                if (cp == end || !isASCIIHexDigit(*cp++, &digit))
                    return UnicodeRegExpError(state);
                n = n*16 + digit;
            }
            if (escape == 'u' && n >= 0xD800 && n <= 0xDBFF && end-cp >= 6 &&
                cp[0] == '\\' && cp[1] == 'u') {
                saved = cp; cp += 2; low = 0;
                for (i=0; i<4; ++i) {
                    if (!isASCIIHexDigit(cp[i], &digit)) break;
                    low = low*16 + digit;
                }
                if (i == 4 && low >= 0xDC00 && low <= 0xDFFF) {
                    n = 0x10000 + ((n-0xD800)<<10) + low-0xDC00;
                    cp += 4;
                } else cp = saved;
            }
        }
        *value = n; break;
      default:
        if (!escape || escape > 127 || !strchr("^$\\.*+?()[]{}|/", (int)escape))
            return UnicodeRegExpError(state);
        *value = escape; break;
    }
    *position = cp;
    return JS_TRUE;
}

static JSBool
UnicodeClassAtom(CompilerState *state, const jschar **position, const jschar *end,
                  uint32 *point, uintN *kind)
{
    const jschar *cp = *position;
    uintN width;
    uint32 escape;
    *kind = 0;
    if (cp == end) return UnicodeRegExpError(state);
    if (*cp != '\\') {
        *point = RegExpPoint(cp, end, JS_TRUE, &width);
        cp += width;
    } else {
        if (++cp == end) return UnicodeRegExpError(state);
        escape = *cp++;
        if (escape && escape < 128 && strchr("dDsSwW", (int)escape)) {
            *kind = escape; *point = 0;
        } else if (escape == 'b') *point = 8;
        else if (escape == '-') *point = '-';
        else if (!UnicodeRegExpEscape(state, &cp, end, escape, point)) return JS_FALSE;
    }
    *position = cp;
    return JS_TRUE;
}

static JSBool
UnicodeWord(uint32 point, JSBool fold)
{
    if (fold) point = js_UnicodeSimpleFold(point);
    return point < 128 && JS_ISWORD(point);
}

/* Sizing and filling use the same parser and canonicalization. */
static JSBool
UnicodeClass(CompilerState *state, const jschar *src, const jschar *end,
             uint8 *bits, uint32 *maximum, JSBool *sense)
{
    uint32 first, last, point, mapped;
    uintN kind, lastKind, work = 0;
    JSBool include, fold = (state->flags & JSREG_FOLD) != 0;
    *maximum = 0; *sense = JS_TRUE;
    if (src < end && *src == '^') { ++src; *sense = JS_FALSE; }
    while (src < end) {
        if (!UnicodeClassAtom(state, &src, end, &first, &kind)) return JS_FALSE;
        last = first;
        if (src+1 < end && *src == '-') {
            ++src;
            if (!UnicodeClassAtom(state, &src, end, &last, &lastKind)) return JS_FALSE;
            if (kind || lastKind || first > last) return UnicodeRegExpError(state);
        }
        if (kind) { first = 0; last = 0x10FFFF; }
        for (point=first; point<=last; ++point) {
            if (!(work++ & 4095) && state->context->branchCallback &&
                !state->context->branchCallback(state->context, NULL)) return JS_FALSE;
            include = JS_TRUE;
            switch (kind) {
              case 'd': case 'D': include = point >= '0' && point <= '9'; break;
              case 'w': case 'W': include = UnicodeWord(point, fold); break;
              case 's': case 'S': include = point <= 0xFFFF && JS_ISSPACE((jschar)point); break;
            }
            if (kind == 'D' || kind == 'W' || kind == 'S') include = !include;
            if (!include) continue;
            mapped = fold ? js_UnicodeSimpleFold(point) : point;
            if (mapped > *maximum) *maximum = mapped;
            if (bits) bits[mapped >> 3] |= 1 << (mapped & 7);
        }
        /* Distinguish a NUL-only class from the empty class. */
        if (*maximum == 0) *maximum = 1;
    }
    return JS_TRUE;
}

/*
 * Calculate the total size of the bitmap required for a class expression.
 */
static JSBool
CalculateBitmapSize(CompilerState *state, RENode *target, const jschar *src,
                    const jschar *end)
{
    uintN max = 0;
    JSBool inRange = JS_FALSE;
    jschar c, rangeStart = 0;
    uintN n, digit, nDigits, i;

    if (state->flags & JSREG_UNICODE) {
        uint32 maximum;
        JSBool sense;
        if (!UnicodeClass(state, src, end, NULL, &maximum, &sense)) return JS_FALSE;
        target->u.ucclass.bmsize = maximum;
        target->u.ucclass.sense = sense;
        return JS_TRUE;
    }
    target->u.ucclass.bmsize = 0;
    target->u.ucclass.sense = JS_TRUE;

    if (src == end)
        return JS_TRUE;

    if (*src == '^') {
        ++src;
        target->u.ucclass.sense = JS_FALSE;
    }

    while (src != end) {
        uintN localMax = 0;
        switch (*src) {
        case '\\':
            ++src;
            c = *src++;
            switch (c) {
            case 'b':
                localMax = 0x8;
                break;
            case 'f':
                localMax = 0xC;
                break;
            case 'n':
                localMax = 0xA;
                break;
            case 'r':
                localMax = 0xD;
                break;
            case 't':
                localMax = 0x9;
                break;
            case 'v':
                localMax = 0xB;
                break;
            case 'c':
                if (src < end && RE_IS_LETTER(*src)) {
                    localMax = (jschar) (*src++ & 0x1F);
                } else {
                    --src;
                    localMax = '\\';
                }
                break;
            case 'x':
                nDigits = 2;
                goto lexHex;
            case 'u':
                nDigits = 4;
lexHex:
                if (state->modern) {
                    if (!CompleteHexEscape(&src, end, nDigits, &n))
                        n = c;
                    localMax = n;
                    break;
                }
                n = 0;
                for (i = 0; (i < nDigits) && (src < end); i++) {
                    c = *src++;
                    if (!isASCIIHexDigit(c, &digit)) {
                        /*
                         * Back off to accepting the original
                         *'\' as a literal.
                         */
                        src -= i + 1;
                        n = '\\';
                        break;
                    }
                    n = (n << 4) | digit;
                }
                localMax = n;
                break;
            case 'd':
                if (inRange || (src < end - 1 && *src == '-')) {
                    if (state->modern) {
                        /* Original ES2015 Annex B ClassAtomInRange falls
                         * back to IdentityEscape for a non-singleton set. */
                        localMax = c;
                        break;
                    }
                    JS_ReportErrorNumber(state->context,
                                         js_GetErrorMessage, NULL,
                                         JSMSG_BAD_CLASS_RANGE);
                    return JS_FALSE;
                }
                localMax = '9';
                break;
            case 'D':
            case 's':
            case 'S':
            case 'w':
            case 'W':
                if (inRange || (src < end - 1 && *src == '-')) {
                    if (state->modern) {
                        /* Original ES2015 Annex B ClassAtomInRange falls
                         * back to IdentityEscape for a non-singleton set. */
                        localMax = c;
                        break;
                    }
                    JS_ReportErrorNumber(state->context,
                                         js_GetErrorMessage, NULL,
                                         JSMSG_BAD_CLASS_RANGE);
                    return JS_FALSE;
                }
                localMax = 65535;
                break;
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
                /*
                 *  This is a non-ECMA extension - decimal escapes (in this
                 *  case, octal!) are supposed to be an error inside class
                 *  ranges, but supported here for backwards compatibility.
                 *
                 */
                n = JS7_UNDEC(c);
                c = *src;
                if ('0' <= c && c <= '7') {
                    src++;
                    n = 8 * n + JS7_UNDEC(c);
                    c = *src;
                    if ('0' <= c && c <= '7') {
                        src++;
                        i = 8 * n + JS7_UNDEC(c);
                        if (i <= 0377)
                            n = i;
                        else
                            src--;
                    }
                }
                localMax = n;
                break;

            default:
                localMax = c;
                break;
            }
            break;
        default:
            localMax = *src++;
            break;
        }
        if (state->flags & JSREG_FOLD) {
            c = JS_MAX(upcase((jschar) localMax), downcase((jschar) localMax));
            if (c > localMax)
                localMax = c;
        }
        if (inRange) {
            if (rangeStart > localMax) {
                JS_ReportErrorNumber(state->context,
                                     js_GetErrorMessage, NULL,
                                     JSMSG_BAD_CLASS_RANGE);
                return JS_FALSE;
            }
            inRange = JS_FALSE;
        } else {
            if (src < end - 1) {
                if (*src == '-') {
                    ++src;
                    inRange = JS_TRUE;
                    rangeStart = (jschar)localMax;
                    continue;
                }
            }
        }
        if (localMax > max)
            max = localMax;
    }
    target->u.ucclass.bmsize = max;
    return JS_TRUE;
}

/*
 *  item:       assertion               An item is either an assertion or
 *              quantatom               a quantified atom.
 *
 *  assertion:  '^'                     Assertions match beginning of string
 *                                      (or line if the class static property
 *                                      RegExp.multiline is true).
 *              '$'                     End of string (or line if the class
 *                                      static property RegExp.multiline is
 *                                      true).
 *              '\b'                    Word boundary (between \w and \W).
 *              '\B'                    Word non-boundary.
 *
 *  quantatom:  atom                    An unquantified atom.
 *              quantatom '{' n ',' m '}'
 *                                      Atom must occur between n and m times.
 *              quantatom '{' n ',' '}' Atom must occur at least n times.
 *              quantatom '{' n '}'     Atom must occur exactly n times.
 *              quantatom '*'           Zero or more times (same as {0,}).
 *              quantatom '+'           One or more times (same as {1,}).
 *              quantatom '?'           Zero or one time (same as {0,1}).
 *
 *              any of which can be optionally followed by '?' for ungreedy
 *
 *  atom:       '(' regexp ')'          A parenthesized regexp (what matched
 *                                      can be addressed using a backreference,
 *                                      see '\' n below).
 *              '.'                     Matches any char except '\n'.
 *              '[' classlist ']'       A character class.
 *              '[' '^' classlist ']'   A negated character class.
 *              '\f'                    Form Feed.
 *              '\n'                    Newline (Line Feed).
 *              '\r'                    Carriage Return.
 *              '\t'                    Horizontal Tab.
 *              '\v'                    Vertical Tab.
 *              '\d'                    A digit (same as [0-9]).
 *              '\D'                    A non-digit.
 *              '\w'                    A word character, [0-9a-z_A-Z].
 *              '\W'                    A non-word character.
 *              '\s'                    A whitespace character, [ \b\f\n\r\t\v].
 *              '\S'                    A non-whitespace character.
 *              '\' n                   A backreference to the nth (n decimal
 *                                      and positive) parenthesized expression.
 *              '\' octal               An octal escape sequence (octal must be
 *                                      two or three digits long, unless it is
 *                                      0 for the null character).
 *              '\x' hex                A hex escape (hex must be two digits).
 *              '\u' unicode            A unicode escape (must be four digits).
 *              '\c' ctrl               A control character, ctrl is a letter.
 *              '\' literalatomchar     Any character except one of the above
 *                                      that follow '\' in an atom.
 *              otheratomchar           Any character not first among the other
 *                                      atom right-hand sides.
 */
static JSBool
ParseTerm(CompilerState *state)
{
    uint32 c = *state->cp++;
    uintN width;
    uintN nDigits;
    uintN num, tmp, n, i;
    const jschar *termStart;

    switch (c) {
    /* assertions and atoms */
    case '^':
        state->result = NewRENode(state, REOP_BOL);
        if (!state->result)
            return JS_FALSE;
        state->progLength++;
        return JS_TRUE;
    case '$':
        state->result = NewRENode(state, REOP_EOL);
        if (!state->result)
            return JS_FALSE;
        state->progLength++;
        return JS_TRUE;
    case '\\':
        if (state->cp >= state->cpend) {
            /* a trailing '\' is an error */
            js_ReportCompileErrorNumber(state->context, state->tokenStream,
                                        JSREPORT_TS | JSREPORT_ERROR,
                                        JSMSG_TRAILING_SLASH);
            return JS_FALSE;
        }
        c = *state->cp++;
        switch (c) {
        /* assertion escapes */
        case 'b' :
            state->result = NewRENode(state, REOP_WBDRY);
            if (!state->result)
                return JS_FALSE;
            state->progLength++;
            return JS_TRUE;
        case 'B':
            state->result = NewRENode(state, REOP_WNONBDRY);
            if (!state->result)
                return JS_FALSE;
            state->progLength++;
            return JS_TRUE;
        /* Decimal escape */
        case '0':
            if (state->flags & JSREG_UNICODE) {
                if (!UnicodeRegExpEscape(state, &state->cp, state->cpend, c, &c))
                    return JS_FALSE;
                goto doFlat;
            }
            /* Give a strict warning. See also the note below. */
            if (!js_ReportCompileErrorNumber(state->context,
                                             state->tokenStream,
                                             JSREPORT_TS |
                                             JSREPORT_WARNING |
                                             JSREPORT_STRICT,
                                             JSMSG_INVALID_BACKREF)) {
                return JS_FALSE;
            }
     doOctal:
            num = 0;
            while (state->cp < state->cpend) {
                c = *state->cp;
                if (c < '0' || '7' < c)
                    break;
                state->cp++;
                tmp = 8 * num + (uintN)JS7_UNDEC(c);
                if (tmp > 0377)
                    break;
                num = tmp;
            }
            c = (jschar)num;
    doFlat:
            state->result = NewRENode(state, REOP_FLAT);
            if (!state->result)
                return JS_FALSE;
            state->result->u.flat.chr = c;
            state->result->u.flat.length = 1;
            state->progLength += (state->flags & JSREG_UNICODE) ? 5 : 3;
            break;
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            termStart = state->cp - 1;
            num = GetDecimalValue(c, state->parenCount, FindParenCount, state);
            if (state->flags & JSREG_FIND_PAREN_ERROR)
                return JS_FALSE;
            if (num == OVERFLOW_VALUE) {
                if (state->flags & JSREG_UNICODE) return UnicodeRegExpError(state);
                /* Give a strict mode warning. */
                if (!js_ReportCompileErrorNumber(state->context,
                                                 state->tokenStream,
                                                 JSREPORT_TS |
                                                 JSREPORT_WARNING |
                                                 JSREPORT_STRICT,
                                                 (c >= '8')
                                                 ? JSMSG_INVALID_BACKREF
                                                 : JSMSG_BAD_BACKREF)) {
                    return JS_FALSE;
                }

                /*
                 * Note: ECMA 262, 15.10.2.9 says that we should throw a syntax
                 * error here. However, for compatibility with IE, we treat the
                 * whole backref as flat if the first character in it is not a
                 * valid octal character, and as an octal escape otherwise.
                 */
                state->cp = termStart;
                if (c >= '8') {
                    if (state->modern) {
                        /* Annex B IdentityEscape consumes the digit, not
                         * the backslash; following decimal digits remain. */
                        state->cp = termStart + 1;
                        goto doFlat;
                    }
                    /* Treat this as flat. termStart - 1 is the \. */
                    c = '\\';
                    goto asFlat;
                }

                /* Treat this as an octal escape. */
                goto doOctal;
            }
            JS_ASSERT(1 <= num && num <= 0x10000);
            state->result = NewRENode(state, REOP_BACKREF);
            if (!state->result)
                return JS_FALSE;
            state->result->u.parenIndex = num - 1;
            state->progLength
                += 1 + GetCompactIndexWidth(state->result->u.parenIndex);
            break;
        /* Control escape */
        case 'f':
            c = 0xC;
            goto doFlat;
        case 'n':
            c = 0xA;
            goto doFlat;
        case 'r':
            c = 0xD;
            goto doFlat;
        case 't':
            c = 0x9;
            goto doFlat;
        case 'v':
            c = 0xB;
            goto doFlat;
        /* Control letter */
        case 'c':
            if (state->flags & JSREG_UNICODE) {
                if (!UnicodeRegExpEscape(state, &state->cp, state->cpend, c, &c))
                    return JS_FALSE;
                goto doFlat;
            }
            if (state->cp < state->cpend && RE_IS_LETTER(*state->cp)) {
                c = (jschar) (*state->cp++ & 0x1F);
            } else {
                /* back off to accepting the original '\' as a literal */
                --state->cp;
                c = '\\';
            }
            goto doFlat;
        /* HexEscapeSequence */
        case 'x':
            nDigits = 2;
            goto lexHex;
        /* UnicodeEscapeSequence */
        case 'u':
            nDigits = 4;
lexHex:
            if (state->flags & JSREG_UNICODE) {
                if (!UnicodeRegExpEscape(state, &state->cp, state->cpend,
                                          nDigits == 4 ? 'u' : 'x', &c))
                    return JS_FALSE;
                goto doFlat;
            }
            if (state->modern) {
                if (!CompleteHexEscape(&state->cp, state->cpend, nDigits, &n))
                    n = nDigits == 4 ? 'u' : 'x';
                c = (jschar)n;
                goto doFlat;
            }
            n = 0;
            for (i = 0; i < nDigits && state->cp < state->cpend; i++) {
                uintN digit;
                c = *state->cp++;
                if (!isASCIIHexDigit(c, &digit)) {
                    /*
                     * Back off to accepting the original 'u' or 'x' as a
                     * literal.
                     */
                    state->cp -= i + 2;
                    n = *state->cp++;
                    break;
                }
                n = (n << 4) | digit;
            }
            c = (jschar) n;
            goto doFlat;
        /* Character class escapes */
        case 'd':
            state->result = NewRENode(state, REOP_DIGIT);
doSimple:
            if (!state->result)
                return JS_FALSE;
            state->progLength++;
            break;
        case 'D':
            state->result = NewRENode(state, REOP_NONDIGIT);
            goto doSimple;
        case 's':
            state->result = NewRENode(state, REOP_SPACE);
            goto doSimple;
        case 'S':
            state->result = NewRENode(state, REOP_NONSPACE);
            goto doSimple;
        case 'w':
            state->result = NewRENode(state, REOP_ALNUM);
            goto doSimple;
        case 'W':
            state->result = NewRENode(state, REOP_NONALNUM);
            goto doSimple;
        /* IdentityEscape */
        default:
            if (state->flags & JSREG_UNICODE) {
                if (!UnicodeRegExpEscape(state, &state->cp, state->cpend, c, &c))
                    return JS_FALSE;
                goto doFlat;
            }
            state->result = NewRENode(state, REOP_FLAT);
            if (!state->result)
                return JS_FALSE;
            state->result->u.flat.chr = c;
            state->result->u.flat.length = 1;
            state->result->kid = (void *) (state->cp - 1);
            state->progLength += (state->flags & JSREG_UNICODE) ? 5 : 3;
            break;
        }
        break;
    case '[':
        state->result = NewRENode(state, REOP_CLASS);
        if (!state->result)
            return JS_FALSE;
        termStart = state->cp;
        state->result->u.ucclass.startIndex = termStart - state->cpbegin;
        for (;;) {
            if (state->cp == state->cpend) {
                js_ReportCompileErrorNumberUC(state->context, state->tokenStream,
                                              JSREPORT_TS | JSREPORT_ERROR,
                                              JSMSG_UNTERM_CLASS, termStart);

                return JS_FALSE;
            }
            if (*state->cp == '\\') {
                state->cp++;
                if (state->cp != state->cpend)
                    state->cp++;
                continue;
            }
            if (*state->cp == ']') {
                state->result->u.ucclass.kidlen = state->cp - termStart;
                break;
            }
            state->cp++;
        }
        for (i = 0; i < CLASS_CACHE_SIZE; i++) {
            if (!state->classCache[i].start) {
                state->classCache[i].start = termStart;
                state->classCache[i].length = state->result->u.ucclass.kidlen;
                state->classCache[i].index = state->classCount;
                break;
            }
            if (state->classCache[i].length ==
                state->result->u.ucclass.kidlen) {
                for (n = 0; ; n++) {
                    if (n == state->classCache[i].length) {
                        state->result->u.ucclass.index
                            = state->classCache[i].index;
                        goto claim;
                    }
                    if (state->classCache[i].start[n] != termStart[n])
                        break;
                }
            }
        }
        state->result->u.ucclass.index = state->classCount++;

    claim:
        /*
         * Call CalculateBitmapSize now as we want any errors it finds
         * to be reported during the parse phase, not at execution.
         */
        if (!CalculateBitmapSize(state, state->result, termStart, state->cp++))
            return JS_FALSE;
        /*
         * Update classBitmapsMem with number of bytes to hold bmsize bits,
         * which is (bitsCount + 7) / 8 or (highest_bit + 1 + 7) / 8
         * or highest_bit / 8 + 1 where highest_bit is u.ucclass.bmsize.
         */
        n = (state->result->u.ucclass.bmsize >> 3) + 1;
        if (n > CLASS_BITMAPS_MEM_LIMIT - state->classBitmapsMem) {
            js_ReportCompileErrorNumber(state->context, state->tokenStream,
                                        JSREPORT_TS | JSREPORT_ERROR,
                                        JSMSG_REGEXP_TOO_COMPLEX);
            return JS_FALSE;
        }
        state->classBitmapsMem += n;
        /* CLASS, <index> */
        state->progLength
            += 1 + GetCompactIndexWidth(state->result->u.ucclass.index);
        break;

    case '.':
        state->result = NewRENode(state, REOP_DOT);
        goto doSimple;

    case '{':
    {
        const jschar *errp = state->cp--;
        intN err;

        err = ParseMinMaxQuantifier(state, JS_TRUE);
        state->cp = errp;

        if (err < 0) {
            if (state->flags & JSREG_UNICODE) return UnicodeRegExpError(state);
            goto asFlat;
        }

        /* FALL THROUGH */
    }
    case '*':
    case '+':
    case '?':
        js_ReportCompileErrorNumberUC(state->context, state->tokenStream,
                                      JSREPORT_TS | JSREPORT_ERROR,
                                      JSMSG_BAD_QUANTIFIER, state->cp - 1);
        return JS_FALSE;
    default:
asFlat:
        if (state->flags & JSREG_UNICODE) {
            if (c == '}' || c == ']') return UnicodeRegExpError(state);
            c = RegExpPoint(state->cp-1, state->cpend, JS_TRUE, &width);
            state->cp += width-1;
        }
        state->result = NewRENode(state, REOP_FLAT);
        if (!state->result)
            return JS_FALSE;
        state->result->u.flat.chr = c;
        state->result->u.flat.length = 1;
        state->result->kid = (void *) (state->cp - 1);
        state->progLength += (state->flags & JSREG_UNICODE) ? 5 : 3;
        break;
    }
    return ParseQuantifier(state);
}

static JSBool
ParseQuantifier(CompilerState *state)
{
    RENode *term;
    term = state->result;
    if (state->cp < state->cpend) {
        switch (*state->cp) {
        case '+':
            state->result = NewRENode(state, REOP_QUANT);
            if (!state->result)
                return JS_FALSE;
            state->result->u.range.min = 1;
            state->result->u.range.max = (uintN)-1;
            /* <PLUS>, <next> ... <ENDCHILD> */
            state->progLength += 4;
            goto quantifier;
        case '*':
            state->result = NewRENode(state, REOP_QUANT);
            if (!state->result)
                return JS_FALSE;
            state->result->u.range.min = 0;
            state->result->u.range.max = (uintN)-1;
            /* <STAR>, <next> ... <ENDCHILD> */
            state->progLength += 4;
            goto quantifier;
        case '?':
            state->result = NewRENode(state, REOP_QUANT);
            if (!state->result)
                return JS_FALSE;
            state->result->u.range.min = 0;
            state->result->u.range.max = 1;
            /* <OPT>, <next> ... <ENDCHILD> */
            state->progLength += 4;
            goto quantifier;
        case '{':       /* balance '}' */
        {
            intN err;
            const jschar *errp = state->cp;

            err = ParseMinMaxQuantifier(state, JS_FALSE);
            if (err == 0)
                goto quantifier;
            if (err == -1) {
                if (state->flags & JSREG_UNICODE) return UnicodeRegExpError(state);
                return JS_TRUE;
            }

            js_ReportCompileErrorNumberUC(state->context,
                                          state->tokenStream,
                                          JSREPORT_TS | JSREPORT_ERROR,
                                          err, errp);
            return JS_FALSE;
        }
        default:;
        }
    }
    return JS_TRUE;

quantifier:
    if ((state->flags & JSREG_UNICODE) &&
        (term->op == REOP_ASSERT || term->op == REOP_ASSERT_NOT))
        return UnicodeRegExpError(state);
    if (state->treeDepth == TREE_DEPTH_MAX) {
        js_ReportCompileErrorNumber(state->context, state->tokenStream,
                                    JSREPORT_TS | JSREPORT_ERROR,
                                    JSMSG_REGEXP_TOO_COMPLEX);
        return JS_FALSE;
    }

    ++state->treeDepth;
    ++state->cp;
    state->result->kid = term;
    if (state->cp < state->cpend && *state->cp == '?') {
        ++state->cp;
        state->result->u.range.greedy = JS_FALSE;
    } else {
        state->result->u.range.greedy = JS_TRUE;
    }
    return JS_TRUE;
}

static intN
ParseMinMaxQuantifier(CompilerState *state, JSBool ignoreValues)
{
    uintN min, max;
    jschar c;
    const jschar *errp = state->cp++;

    c = *state->cp;
    if (JS7_ISDEC(c)) {
        ++state->cp;
        min = GetDecimalValue(c, 0xFFFF, NULL, state);
        c = *state->cp;

        if (!ignoreValues && min == OVERFLOW_VALUE)
            return JSMSG_MIN_TOO_BIG;

        if (c == ',') {
            c = *++state->cp;
            if (JS7_ISDEC(c)) {
                ++state->cp;
                max = GetDecimalValue(c, 0xFFFF, NULL, state);
                c = *state->cp;
                if (!ignoreValues && max == OVERFLOW_VALUE)
                    return JSMSG_MAX_TOO_BIG;
                if (!ignoreValues && min > max)
                    return JSMSG_OUT_OF_ORDER;
            } else {
                max = (uintN)-1;
            }
        } else {
            max = min;
        }
        if (c == '}') {
            state->result = NewRENode(state, REOP_QUANT);
            if (!state->result)
                return JS_FALSE;
            state->result->u.range.min = min;
            state->result->u.range.max = max;
            /*
             * QUANT, <min>, <max>, <next> ... <ENDCHILD>
             * where <max> is written as compact(max+1) to make
             * (uintN)-1 sentinel to occupy 1 byte, not width_of(max)+1.
             */
            state->progLength += (1 + GetCompactIndexWidth(min)
                                  + GetCompactIndexWidth(max + 1)
                                  +3);
            return 0;
        }
    }

    state->cp = errp;
    return -1;
}

static JSBool
SetForwardJumpOffset(jsbytecode *jump, jsbytecode *target)
{
    ptrdiff_t offset = target - jump;

    /* Check that target really points forward. */
    JS_ASSERT(offset >= 2);
    if ((size_t)offset > OFFSET_MAX)
        return JS_FALSE;

    jump[0] = JUMP_OFFSET_HI(offset);
    jump[1] = JUMP_OFFSET_LO(offset);
    return JS_TRUE;
}

/*
 * Generate bytecode for the tree rooted at t using an explicit stack instead
 * of recursion.
 */
static jsbytecode *
EmitREBytecode(CompilerState *state, JSRegExp *re, size_t treeDepth,
               jsbytecode *pc, RENode *t)
{
    EmitStateStackEntry *emitStateSP, *emitStateStack;
    RECharSet *charSet;
    REOp op;

    if (treeDepth == 0) {
        emitStateStack = NULL;
    } else {
        emitStateStack =
            (EmitStateStackEntry *)JS_malloc(state->context,
                                             sizeof(EmitStateStackEntry) *
                                             treeDepth);
        if (!emitStateStack)
            return NULL;
    }
    emitStateSP = emitStateStack;
    op = t->op;

    for (;;) {
        *pc++ = op;
        switch (op) {
        case REOP_EMPTY:
            --pc;
            break;

        case REOP_ALTPREREQ2:
        case REOP_ALTPREREQ:
            JS_ASSERT(emitStateSP);
            emitStateSP->altHead = pc - 1;
            emitStateSP->endTermFixup = pc;
            pc += OFFSET_LEN;
            SET_ARG(pc, t->u.altprereq.ch1);
            pc += ARG_LEN;
            SET_ARG(pc, t->u.altprereq.ch2);
            pc += ARG_LEN;

            emitStateSP->nextAltFixup = pc;    /* offset to next alternate */
            pc += OFFSET_LEN;

            emitStateSP->continueNode = t;
            emitStateSP->continueOp = REOP_JUMP;
            emitStateSP->jumpToJumpFlag = JS_FALSE;
            ++emitStateSP;
            JS_ASSERT((size_t)(emitStateSP - emitStateStack) <= treeDepth);
            t = (RENode *) t->kid;
            op = t->op;
            continue;

        case REOP_JUMP:
            emitStateSP->nextTermFixup = pc;    /* offset to following term */
            pc += OFFSET_LEN;
            if (!SetForwardJumpOffset(emitStateSP->nextAltFixup, pc))
                goto jump_too_big;
            emitStateSP->continueOp = REOP_ENDALT;
            ++emitStateSP;
            JS_ASSERT((size_t)(emitStateSP - emitStateStack) <= treeDepth);
            t = t->u.kid2;
            op = t->op;
            continue;

        case REOP_ENDALT:
            /*
             * If we already patched emitStateSP->nextTermFixup to jump to
             * a nearer jump, to avoid 16-bit immediate offset overflow, we
             * are done here.
             */
            if (emitStateSP->jumpToJumpFlag)
                break;

            /*
             * Fix up the REOP_JUMP offset to go to the op after REOP_ENDALT.
             * REOP_ENDALT is executed only on successful match of the last
             * alternate in a group.
             */
            if (!SetForwardJumpOffset(emitStateSP->nextTermFixup, pc))
                goto jump_too_big;
            if (t->op != REOP_ALT) {
                if (!SetForwardJumpOffset(emitStateSP->endTermFixup, pc))
                    goto jump_too_big;
            }

            /*
             * If the program is bigger than the REOP_JUMP offset range, then
             * we must check for alternates before this one that are part of
             * the same group, and fix up their jump offsets to target jumps
             * close enough to fit in a 16-bit unsigned offset immediate.
             */
            if ((size_t)(pc - re->program) > OFFSET_MAX &&
                emitStateSP > emitStateStack) {
                EmitStateStackEntry *esp, *esp2;
                jsbytecode *alt, *jump;
                ptrdiff_t span, header;

                esp2 = emitStateSP;
                alt = esp2->altHead;
                for (esp = esp2 - 1; esp >= emitStateStack; --esp) {
                    if (esp->continueOp == REOP_ENDALT &&
                        !esp->jumpToJumpFlag &&
                        esp->nextTermFixup + OFFSET_LEN == alt &&
                        (size_t)(pc - ((esp->continueNode->op != REOP_ALT)
                                       ? esp->endTermFixup
                                       : esp->nextTermFixup)) > OFFSET_MAX) {
                        alt = esp->altHead;
                        jump = esp->nextTermFixup;

                        /*
                         * The span must be 1 less than the distance from
                         * jump offset to jump offset, so we actually jump
                         * to a REOP_JUMP bytecode, not to its offset!
                         */
                        for (;;) {
                            JS_ASSERT(jump < esp2->nextTermFixup);
                            span = esp2->nextTermFixup - jump - 1;
                            if ((size_t)span <= OFFSET_MAX)
                                break;
                            do {
                                if (--esp2 == esp)
                                    goto jump_too_big;
                            } while (esp2->continueOp != REOP_ENDALT);
                        }

                        jump[0] = JUMP_OFFSET_HI(span);
                        jump[1] = JUMP_OFFSET_LO(span);

                        if (esp->continueNode->op != REOP_ALT) {
                            /*
                             * We must patch the offset at esp->endTermFixup
                             * as well, for the REOP_ALTPREREQ{,2} opcodes.
                             * If we're unlucky and endTermFixup is more than
                             * OFFSET_MAX bytes from its target, we cheat by
                             * jumping 6 bytes to the jump whose offset is at
                             * esp->nextTermFixup, which has the same target.
                             */
                            jump = esp->endTermFixup;
                            header = esp->nextTermFixup - jump;
                            span += header;
                            if ((size_t)span > OFFSET_MAX)
                                span = header;

                            jump[0] = JUMP_OFFSET_HI(span);
                            jump[1] = JUMP_OFFSET_LO(span);
                        }

                        esp->jumpToJumpFlag = JS_TRUE;
                    }
                }
            }
            break;

        case REOP_ALT:
            JS_ASSERT(emitStateSP);
            emitStateSP->altHead = pc - 1;
            emitStateSP->nextAltFixup = pc;     /* offset to next alternate */
            pc += OFFSET_LEN;
            emitStateSP->continueNode = t;
            emitStateSP->continueOp = REOP_JUMP;
            emitStateSP->jumpToJumpFlag = JS_FALSE;
            ++emitStateSP;
            JS_ASSERT((size_t)(emitStateSP - emitStateStack) <= treeDepth);
            t = t->kid;
            op = t->op;
            continue;

        case REOP_FLAT:
            /*
             * Coalesce FLATs if possible and if it would not increase bytecode
             * beyond preallocated limit. The latter happens only when bytecode
             * size for coalesced string with offset p and length 2 exceeds 6
             * bytes preallocated for 2 single char nodes, i.e. when
             * 1 + GetCompactIndexWidth(p) + GetCompactIndexWidth(2) > 6 or
             * GetCompactIndexWidth(p) > 4.
             * Since when GetCompactIndexWidth(p) <= 4 coalescing of 3 or more
             * nodes strictly decreases bytecode size, the check has to be
             * done only for the first coalescing.
             */
            if (!(state->flags & JSREG_UNICODE) && t->kid &&
                GetCompactIndexWidth((jschar *)t->kid - state->cpbegin) <= 4)
            {
                while (t->next &&
                       t->next->op == REOP_FLAT &&
                       (jschar*)t->kid + t->u.flat.length ==
                       (jschar*)t->next->kid) {
                    t->u.flat.length += t->next->u.flat.length;
                    t->next = t->next->next;
                }
            }
            if (t->kid && t->u.flat.length > 1) {
                pc[-1] = (state->flags & JSREG_FOLD) ? REOP_FLATi : REOP_FLAT;
                pc = WriteCompactIndex(pc, (jschar *)t->kid - state->cpbegin);
                pc = WriteCompactIndex(pc, t->u.flat.length);
            } else if (state->flags & JSREG_UNICODE) {
                pc[-1] = REOP_UCFLAT32;
                SET_ARG(pc, t->u.flat.chr >> 16); pc += ARG_LEN;
                SET_ARG(pc, t->u.flat.chr & 0xFFFF); pc += ARG_LEN;
            } else if (t->u.flat.chr < 256) {
                pc[-1] = (state->flags & JSREG_FOLD) ? REOP_FLAT1i : REOP_FLAT1;
                *pc++ = (jsbytecode) t->u.flat.chr;
            } else {
                pc[-1] = (state->flags & JSREG_FOLD)
                         ? REOP_UCFLAT1i
                         : REOP_UCFLAT1;
                SET_ARG(pc, t->u.flat.chr);
                pc += ARG_LEN;
            }
            break;

        case REOP_LPAREN:
            JS_ASSERT(emitStateSP);
            pc = WriteCompactIndex(pc, t->u.parenIndex);
            emitStateSP->continueNode = t;
            emitStateSP->continueOp = REOP_RPAREN;
            ++emitStateSP;
            JS_ASSERT((size_t)(emitStateSP - emitStateStack) <= treeDepth);
            t = (RENode *) t->kid;
            op = t->op;
            continue;

        case REOP_RPAREN:
            pc = WriteCompactIndex(pc, t->u.parenIndex);
            break;

        case REOP_BACKREF:
            pc = WriteCompactIndex(pc, t->u.parenIndex);
            break;

        case REOP_ASSERT:
            JS_ASSERT(emitStateSP);
            emitStateSP->nextTermFixup = pc;
            pc += OFFSET_LEN;
            emitStateSP->continueNode = t;
            emitStateSP->continueOp = REOP_ASSERTTEST;
            ++emitStateSP;
            JS_ASSERT((size_t)(emitStateSP - emitStateStack) <= treeDepth);
            t = (RENode *) t->kid;
            op = t->op;
            continue;

        case REOP_ASSERTTEST:
        case REOP_ASSERTNOTTEST:
            if (!SetForwardJumpOffset(emitStateSP->nextTermFixup, pc))
                goto jump_too_big;
            break;

        case REOP_ASSERT_NOT:
            JS_ASSERT(emitStateSP);
            emitStateSP->nextTermFixup = pc;
            pc += OFFSET_LEN;
            emitStateSP->continueNode = t;
            emitStateSP->continueOp = REOP_ASSERTNOTTEST;
            ++emitStateSP;
            JS_ASSERT((size_t)(emitStateSP - emitStateStack) <= treeDepth);
            t = (RENode *) t->kid;
            op = t->op;
            continue;

        case REOP_QUANT:
            JS_ASSERT(emitStateSP);
            if (t->u.range.min == 0 && t->u.range.max == (uintN)-1) {
                pc[-1] = (t->u.range.greedy) ? REOP_STAR : REOP_MINIMALSTAR;
            } else if (t->u.range.min == 0 && t->u.range.max == 1) {
                pc[-1] = (t->u.range.greedy) ? REOP_OPT : REOP_MINIMALOPT;
            } else if (t->u.range.min == 1 && t->u.range.max == (uintN) -1) {
                pc[-1] = (t->u.range.greedy) ? REOP_PLUS : REOP_MINIMALPLUS;
            } else {
                if (!t->u.range.greedy)
                    pc[-1] = REOP_MINIMALQUANT;
                pc = WriteCompactIndex(pc, t->u.range.min);
                /*
                 * Write max + 1 to avoid using size_t(max) + 1 bytes
                 * for (uintN)-1 sentinel.
                 */
                pc = WriteCompactIndex(pc, t->u.range.max + 1);
            }
            emitStateSP->nextTermFixup = pc;
            pc += OFFSET_LEN;
            emitStateSP->continueNode = t;
            emitStateSP->continueOp = REOP_ENDCHILD;
            ++emitStateSP;
            JS_ASSERT((size_t)(emitStateSP - emitStateStack) <= treeDepth);
            t = (RENode *) t->kid;
            op = t->op;
            continue;

        case REOP_ENDCHILD:
            if (!SetForwardJumpOffset(emitStateSP->nextTermFixup, pc))
                goto jump_too_big;
            break;

        case REOP_CLASS:
            if (!t->u.ucclass.sense)
                pc[-1] = REOP_NCLASS;
            pc = WriteCompactIndex(pc, t->u.ucclass.index);
            charSet = &re->classList[t->u.ucclass.index];
            charSet->converted = JS_FALSE;
            charSet->length = t->u.ucclass.bmsize;
            charSet->u.src.startIndex = t->u.ucclass.startIndex;
            charSet->u.src.length = t->u.ucclass.kidlen;
            charSet->sense = t->u.ucclass.sense;
            break;

        default:
            break;
        }

        t = t->next;
        if (t) {
            op = t->op;
        } else {
            if (emitStateSP == emitStateStack)
                break;
            --emitStateSP;
            t = emitStateSP->continueNode;
            op = emitStateSP->continueOp;
        }
    }

  cleanup:
    if (emitStateStack)
        JS_free(state->context, emitStateStack);
    return pc;

  jump_too_big:
    js_ReportCompileErrorNumber(state->context, state->tokenStream,
                                JSREPORT_TS | JSREPORT_ERROR,
                                JSMSG_REGEXP_TOO_COMPLEX);
    pc = NULL;
    goto cleanup;
}


static JSRegExp *
NewRegExpWithEdition(JSContext *cx, JSTokenStream *ts,
                      JSString *str, uintN flags, JSBool flat, JSBool modern)
{
    JSRegExp *re;
    void *mark;
    CompilerState state;
    size_t resize;
    jsbytecode *endPC;
    uintN i;
    size_t len;
    JSTempValueRooter sourceRoot;

    JS_PUSH_TEMP_ROOT_STRING(cx, str, &sourceRoot);
    re = NULL;
    mark = JS_ARENA_MARK(&cx->tempPool);
    len = JSSTRING_LENGTH(str);

    state.modern = modern;
    state.context = cx;
    state.tokenStream = ts;
    state.cp = js_UndependString(cx, str);
    if (!state.cp)
        goto out;
    state.cpbegin = state.cp;
    state.cpend = state.cp + len;
    state.flags = flags;
    state.parenCount = 0;
    state.classCount = 0;
    state.progLength = 0;
    state.treeDepth = 0;
    state.classBitmapsMem = 0;
    for (i = 0; i < CLASS_CACHE_SIZE; i++)
        state.classCache[i].start = NULL;

    if (len != 0 && flat) {
        state.result = NewRENode(&state, REOP_FLAT);
        state.result->u.flat.chr = *state.cpbegin;
        state.result->u.flat.length = len;
        state.result->kid = (void *) state.cpbegin;
        /* Flat bytecode: REOP_FLAT compact(string_offset) compact(len). */
        state.progLength += (flags & JSREG_UNICODE) && len == 1 ? 5 :
                            1 + GetCompactIndexWidth(0) + GetCompactIndexWidth(len);
    } else {
        if (!ParseRegExp(&state))
            goto out;
    }
    resize = offsetof(JSRegExp, program) + state.progLength + 1;
    re = (JSRegExp *) JS_malloc(cx, resize);
    if (!re)
        goto out;

    re->nrefs = 1;
    JS_ASSERT(state.classBitmapsMem <= CLASS_BITMAPS_MEM_LIMIT);
    re->classCount = state.classCount;
    if (re->classCount) {
        re->classList = (RECharSet *)
            JS_malloc(cx, re->classCount * sizeof(RECharSet));
        if (!re->classList) {
            js_DestroyRegExp(cx, re);
            re = NULL;
            goto out;
        }
        for (i = 0; i < re->classCount; i++) {
            re->classList[i].converted = JS_FALSE;
            re->classList[i].modern = modern;
        }
    } else {
        re->classList = NULL;
    }
    endPC = EmitREBytecode(&state, re, state.treeDepth, re->program, state.result);
    if (!endPC) {
        js_DestroyRegExp(cx, re);
        re = NULL;
        goto out;
    }
    *endPC++ = REOP_END;
    /*
     * Check whether size was overestimated and shrink using realloc.
     * This is safe since no pointers to newly parsed regexp or its parts
     * besides re exist here.
     */
    if ((size_t)(endPC - re->program) != state.progLength + 1) {
        JSRegExp *tmp;
        JS_ASSERT((size_t)(endPC - re->program) < state.progLength + 1);
        resize = offsetof(JSRegExp, program) + (endPC - re->program);
        tmp = (JSRegExp *) JS_realloc(cx, re, resize);
        if (tmp)
            re = tmp;
    }

    re->flags = flags;
    re->modern = modern;
    re->cloneIndex = 0;
    re->parenCount = state.parenCount;
    re->source = str;
    if (flags & JSREG_UNICODE) {
        for (i=0; i<re->classCount; ++i) {
            RECharSet *set = &re->classList[i];
            const jschar *src = state.cpbegin + set->u.src.startIndex;
            const jschar *end = src + set->u.src.length;
            uint32 maximum;
            JSBool sense;
            size_t size = (set->length >> 3) + 1;
            uint8 *bits = (uint8 *)JS_malloc(cx, size);
            if (!bits) { js_DestroyRegExp(cx, re); re = NULL; goto out; }
            memset(bits, 0, size);
            if (!UnicodeClass(&state, src, end, bits, &maximum, &sense)) {
                JS_free(cx, bits); js_DestroyRegExp(cx, re); re = NULL; goto out;
            }
            set->u.bits = bits;
            set->converted = JS_TRUE;
        }
    }

out:
    JS_POP_TEMP_ROOT(cx, &sourceRoot);
    JS_ARENA_RELEASE(&cx->tempPool, mark);
    return re;
}

JSRegExp *
js_NewRegExp(JSContext *cx, JSTokenStream *ts,
             JSString *str, uintN flags, JSBool flat)
{
    return NewRegExpWithEdition(cx, ts, str, flags, flat,
                                JS_VERSION_IS_ES2015(cx));
}

static JSRegExp *
NewRegExpOpt(JSContext *cx, JSTokenStream *ts,
              JSString *str, JSString *opt, JSBool flat, JSBool modern)
{
    uintN flags, flag;
    jschar *s;
    size_t i, n;
    char charBuf[2];

    flags = 0;
    if (opt) {
        s = JSSTRING_CHARS(opt);
        for (i = 0, n = JSSTRING_LENGTH(opt); i < n; i++) {
            switch (s[i]) {
            case 'g':
                flag = JSREG_GLOB;
                break;
            case 'i':
                flag = JSREG_FOLD;
                break;
            case 'm':
                flag = JSREG_MULTILINE;
                break;
            case 'u':
                if (modern) { flag = JSREG_UNICODE; break; }
                goto badFlag;
            case 'y':
                if (modern) { flag = JSREG_STICKY; break; }
                /* Fall through for legacy source. */
            default:
              badFlag:
                charBuf[0] = (char)s[i];
                charBuf[1] = '\0';
                js_ReportCompileErrorNumber(cx, ts,
                                            JSREPORT_TS | JSREPORT_ERROR,
                                            JSMSG_BAD_FLAG, charBuf);
                return NULL;
            }
            if (flags & flag) {
                charBuf[0] = (char)s[i];
                charBuf[1] = '\0';
                js_ReportCompileErrorNumber(cx, ts, JSREPORT_TS | JSREPORT_ERROR,
                                             JSMSG_BAD_FLAG, charBuf);
                return NULL;
            }
            flags |= flag;
        }
    }
    return NewRegExpWithEdition(cx, ts, str, flags, flat, modern);
}

JSRegExp *
js_NewRegExpOpt(JSContext *cx, JSTokenStream *ts,
                JSString *str, JSString *opt, JSBool flat)
{
    return NewRegExpOpt(cx, ts, str, opt, flat, JS_VERSION_IS_ES2015(cx));
}

/*
 * Save the current state of the match - the position in the input
 * text as well as the position in the bytecode. The state of any
 * parent expressions is also saved (preceding state).
 * Contents of parenCount parentheses from parenIndex are also saved.
 */
static REBackTrackData *
PushBackTrackState(REGlobalData *gData, REOp op,
                   jsbytecode *target, REMatchState *x, const jschar *cp,
                   size_t parenIndex, size_t parenCount)
{
    size_t i;
    REBackTrackData *result =
        (REBackTrackData *) ((char *)gData->backTrackSP + gData->cursz);

    size_t sz = sizeof(REBackTrackData) +
                gData->stateStackTop * sizeof(REProgState) +
                parenCount * sizeof(RECapture);

    ptrdiff_t btsize = gData->backTrackStackSize;
    ptrdiff_t btincr = ((char *)result + sz) -
                       ((char *)gData->backTrackStack + btsize);

    if (btincr > 0) {
        ptrdiff_t offset = (char *)result - (char *)gData->backTrackStack;

        btincr = JS_ROUNDUP(btincr, btsize);
        JS_ARENA_GROW_CAST(gData->backTrackStack, REBackTrackData *,
                           &gData->pool, btsize, btincr);
        if (!gData->backTrackStack) {
            JS_ReportOutOfMemory(gData->cx);
            gData->ok = JS_FALSE;
            return NULL;
        }
        gData->backTrackStackSize = btsize + btincr;
        result = (REBackTrackData *) ((char *)gData->backTrackStack + offset);
    }
    gData->backTrackSP = result;
    result->sz = gData->cursz;
    gData->cursz = sz;

    result->backtrack_op = op;
    result->backtrack_pc = target;
    result->cp = cp;
    result->parenCount = parenCount;

    result->saveStateStackTop = gData->stateStackTop;
    JS_ASSERT(gData->stateStackTop);
    memcpy(result + 1, gData->stateStack,
           sizeof(REProgState) * result->saveStateStackTop);

    if (parenCount != 0) {
        result->parenIndex = parenIndex;
        memcpy((char *)(result + 1) +
               sizeof(REProgState) * result->saveStateStackTop,
               &x->parens[parenIndex],
               sizeof(RECapture) * parenCount);
        for (i = 0; i != parenCount; i++)
            x->parens[parenIndex + i].index = -1;
    }

    return result;
}


/*
 *   Consecutive literal characters.
 */
#if 0
static REMatchState *
FlatNMatcher(REGlobalData *gData, REMatchState *x, jschar *matchChars,
             size_t length)
{
    size_t i;
    if (length > gData->cpend - x->cp)
        return NULL;
    for (i = 0; i != length; i++) {
        if (matchChars[i] != x->cp[i])
            return NULL;
    }
    x->cp += length;
    return x;
}
#endif

static REMatchState *
UnicodeSequenceMatcher(REGlobalData *gData, REMatchState *x,
                       const jschar *source, size_t length)
{
    const jschar *left = source, *end = source+length, *right = x->cp;
    uint32 a, b;
    uintN aw, bw;
    while (left < end) {
        if (right >= gData->cpend) return NULL;
        a = RegExpPoint(left, end, JS_TRUE, &aw);
        b = RegExpPoint(right, gData->cpend, JS_TRUE, &bw);
        if (gData->regexp->flags & JSREG_FOLD) {
            a = js_UnicodeSimpleFold(a); b = js_UnicodeSimpleFold(b);
        }
        if (a != b) return NULL;
        left += aw; right += bw;
    }
    x->cp = right;
    return x;
}

static REMatchState *
FlatNIMatcher(REGlobalData *gData, REMatchState *x, jschar *matchChars,
              size_t length)
{
    size_t i;
    JS_ASSERT(gData->cpend >= x->cp);
    if (gData->regexp->flags & JSREG_UNICODE)
        return UnicodeSequenceMatcher(gData, x, matchChars, length);
    if (length > (size_t)(gData->cpend - x->cp))
        return NULL;
    for (i = 0; i != length; i++) {
        if (upcase(matchChars[i]) != upcase(x->cp[i]))
            return NULL;
    }
    x->cp += length;
    return x;
}

/*
 * 1. Evaluate DecimalEscape to obtain an EscapeValue E.
 * 2. If E is not a character then go to step 6.
 * 3. Let ch be E's character.
 * 4. Let A be a one-element RECharSet containing the character ch.
 * 5. Call CharacterSetMatcher(A, false) and return its Matcher result.
 * 6. E must be an integer. Let n be that integer.
 * 7. If n=0 or n>NCapturingParens then throw a SyntaxError exception.
 * 8. Return an internal Matcher closure that takes two arguments, a State x
 *    and a Continuation c, and performs the following:
 *     1. Let cap be x's captures internal array.
 *     2. Let s be cap[n].
 *     3. If s is undefined, then call c(x) and return its result.
 *     4. Let e be x's endIndex.
 *     5. Let len be s's length.
 *     6. Let f be e+len.
 *     7. If f>InputLength, return failure.
 *     8. If there exists an integer i between 0 (inclusive) and len (exclusive)
 *        such that Canonicalize(s[i]) is not the same character as
 *        Canonicalize(Input [e+i]), then return failure.
 *     9. Let y be the State (f, cap).
 *     10. Call c(y) and return its result.
 */
static REMatchState *
BackrefMatcher(REGlobalData *gData, REMatchState *x, size_t parenIndex)
{
    size_t len, i;
    const jschar *parenContent;
    RECapture *cap = &x->parens[parenIndex];

    if (cap->index == -1)
        return x;

    len = cap->length;
    if (gData->regexp->flags & JSREG_UNICODE)
        return UnicodeSequenceMatcher(gData, x, gData->cpbegin+cap->index, len);
    if (x->cp + len > gData->cpend)
        return NULL;

    parenContent = &gData->cpbegin[cap->index];
    if (gData->regexp->flags & JSREG_FOLD) {
        for (i = 0; i < len; i++) {
            if (upcase(parenContent[i]) != upcase(x->cp[i]))
                return NULL;
        }
    } else {
        for (i = 0; i < len; i++) {
            if (parenContent[i] != x->cp[i])
                return NULL;
        }
    }
    x->cp += len;
    return x;
}


/* Add a single character to the RECharSet */
static void
AddCharacterToCharSet(RECharSet *cs, jschar c)
{
    uintN byteIndex = (uintN)(c >> 3);
    JS_ASSERT(c <= cs->length);
    cs->u.bits[byteIndex] |= 1 << (c & 0x7);
}


/* Add a character range, c1 to c2 (inclusive) to the RECharSet */
static void
AddCharacterRangeToCharSet(RECharSet *cs, jschar c1, jschar c2)
{
    uintN i;

    uintN byteIndex1 = (uintN)(c1 >> 3);
    uintN byteIndex2 = (uintN)(c2 >> 3);

    JS_ASSERT((c2 <= cs->length) && (c1 <= c2));

    c1 &= 0x7;
    c2 &= 0x7;

    if (byteIndex1 == byteIndex2) {
        cs->u.bits[byteIndex1] |= ((uint8)0xFF >> (7 - (c2 - c1))) << c1;
    } else {
        cs->u.bits[byteIndex1] |= 0xFF << c1;
        for (i = byteIndex1 + 1; i < byteIndex2; i++)
            cs->u.bits[i] = 0xFF;
        cs->u.bits[byteIndex2] |= (uint8)0xFF >> (7 - c2);
    }
}

/* Compile the source of the class into a RECharSet */
static JSBool
ProcessCharSet(REGlobalData *gData, RECharSet *charSet)
{
    const jschar *src, *end;
    JSBool inRange = JS_FALSE;
    jschar rangeStart = 0;
    uintN byteLength, n;
    jschar c, thisCh;
    intN nDigits, i;

    JS_ASSERT(!charSet->converted);
    /*
     * Assert that startIndex and length points to chars inside [] inside
     * source string.
     */
    JS_ASSERT(1 <= charSet->u.src.startIndex);
    JS_ASSERT(charSet->u.src.startIndex
              < JSSTRING_LENGTH(gData->regexp->source));
    JS_ASSERT(charSet->u.src.length <= JSSTRING_LENGTH(gData->regexp->source)
                                       - 1 - charSet->u.src.startIndex);

    charSet->converted = JS_TRUE;
    src = JSSTRING_CHARS(gData->regexp->source) + charSet->u.src.startIndex;
    end = src + charSet->u.src.length;
    JS_ASSERT(src[-1] == '[');
    JS_ASSERT(end[0] == ']');

    byteLength = (charSet->length >> 3) + 1;
    charSet->u.bits = (uint8 *)JS_malloc(gData->cx, byteLength);
    if (!charSet->u.bits) {
        JS_ReportOutOfMemory(gData->cx);
        gData->ok = JS_FALSE;
        return JS_FALSE;
    }
    memset(charSet->u.bits, 0, byteLength);

    if (src == end)
        return JS_TRUE;

    if (*src == '^') {
        JS_ASSERT(charSet->sense == JS_FALSE);
        ++src;
    } else {
        JS_ASSERT(charSet->sense == JS_TRUE);
    }

    while (src != end) {
        switch (*src) {
        case '\\':
            ++src;
            c = *src++;
            switch (c) {
            case 'b':
                thisCh = 0x8;
                break;
            case 'f':
                thisCh = 0xC;
                break;
            case 'n':
                thisCh = 0xA;
                break;
            case 'r':
                thisCh = 0xD;
                break;
            case 't':
                thisCh = 0x9;
                break;
            case 'v':
                thisCh = 0xB;
                break;
            case 'c':
                if (src < end && JS_ISWORD(*src)) {
                    thisCh = (jschar)(*src++ & 0x1F);
                } else {
                    --src;
                    thisCh = '\\';
                }
                break;
            case 'x':
                nDigits = 2;
                goto lexHex;
            case 'u':
                nDigits = 4;
            lexHex:
                if (charSet->modern) {
                    if (!CompleteHexEscape(&src, end, nDigits, &n))
                        n = c;
                    thisCh = (jschar)n;
                    break;
                }
                n = 0;
                for (i = 0; (i < nDigits) && (src < end); i++) {
                    uintN digit;
                    c = *src++;
                    if (!isASCIIHexDigit(c, &digit)) {
                        /*
                         * Back off to accepting the original '\'
                         * as a literal
                         */
                        src -= i + 1;
                        n = '\\';
                        break;
                    }
                    n = (n << 4) | digit;
                }
                thisCh = (jschar)n;
                break;
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
                /*
                 *  This is a non-ECMA extension - decimal escapes (in this
                 *  case, octal!) are supposed to be an error inside class
                 *  ranges, but supported here for backwards compatibility.
                 */
                n = JS7_UNDEC(c);
                c = *src;
                if ('0' <= c && c <= '7') {
                    src++;
                    n = 8 * n + JS7_UNDEC(c);
                    c = *src;
                    if ('0' <= c && c <= '7') {
                        src++;
                        i = 8 * n + JS7_UNDEC(c);
                        if (i <= 0377)
                            n = i;
                        else
                            src--;
                    }
                }
                thisCh = (jschar)n;
                break;

            case 'd':
            case 'D':
            case 's':
            case 'S':
            case 'w':
            case 'W':
                /* Such endpoints are accepted only by the ES2015 parser;
                 * retain that decision when lazily building the bitmap from
                 * a different caller edition. Unicode uses UnicodeClass. */
                if (inRange || (src < end - 1 && *src == '-')) {
                    thisCh = c;
                    break;
                }
                if (c == 'D') goto class_non_digit;
                if (c == 's') goto class_space;
                if (c == 'S') goto class_non_space;
                if (c == 'w') goto class_word;
                if (c == 'W') goto class_non_word;
                AddCharacterRangeToCharSet(charSet, '0', '9');
                continue;   /* don't need range processing */
            class_non_digit:
                AddCharacterRangeToCharSet(charSet, 0, '0' - 1);
                AddCharacterRangeToCharSet(charSet,
                                           (jschar)('9' + 1),
                                           (jschar)charSet->length);
                continue;
            class_space:
                for (i = (intN)charSet->length; i >= 0; i--)
                    if (JS_ISSPACE(i))
                        AddCharacterToCharSet(charSet, (jschar)i);
                continue;
            class_non_space:
                for (i = (intN)charSet->length; i >= 0; i--)
                    if (!JS_ISSPACE(i))
                        AddCharacterToCharSet(charSet, (jschar)i);
                continue;
            class_word:
                for (i = (intN)charSet->length; i >= 0; i--)
                    if (JS_ISWORD(i))
                        AddCharacterToCharSet(charSet, (jschar)i);
                continue;
            class_non_word:
                for (i = (intN)charSet->length; i >= 0; i--)
                    if (!JS_ISWORD(i))
                        AddCharacterToCharSet(charSet, (jschar)i);
                continue;
            default:
                thisCh = c;
                break;

            }
            break;

        default:
            thisCh = *src++;
            break;

        }
        if (inRange) {
            if (gData->regexp->flags & JSREG_FOLD) {
                AddCharacterRangeToCharSet(charSet, upcase(rangeStart),
                                                    upcase(thisCh));
                AddCharacterRangeToCharSet(charSet, downcase(rangeStart),
                                                    downcase(thisCh));
            } else {
                AddCharacterRangeToCharSet(charSet, rangeStart, thisCh);
            }
            inRange = JS_FALSE;
        } else {
            if (gData->regexp->flags & JSREG_FOLD) {
                AddCharacterToCharSet(charSet, upcase(thisCh));
                AddCharacterToCharSet(charSet, downcase(thisCh));
            } else {
                AddCharacterToCharSet(charSet, thisCh);
            }
            if (src < end - 1) {
                if (*src == '-') {
                    ++src;
                    inRange = JS_TRUE;
                    rangeStart = thisCh;
                }
            }
        }
    }
    return JS_TRUE;
}

void
js_DestroyRegExp(JSContext *cx, JSRegExp *re)
{
    if (JS_ATOMIC_DECREMENT(&re->nrefs) == 0) {
        if (re->classList) {
            uintN i;
            for (i = 0; i < re->classCount; i++) {
                if (re->classList[i].converted)
                    JS_free(cx, re->classList[i].u.bits);
                re->classList[i].u.bits = NULL;
            }
            JS_free(cx, re->classList);
        }
        JS_free(cx, re);
    }
}

static JSBool
ReallocStateStack(REGlobalData *gData)
{
    size_t limit = gData->stateStackLimit;
    size_t sz = sizeof(REProgState) * limit;

    JS_ARENA_GROW_CAST(gData->stateStack, REProgState *, &gData->pool, sz, sz);
    if (!gData->stateStack) {
        gData->ok = JS_FALSE;
        return JS_FALSE;
    }
    gData->stateStackLimit = limit + limit;
    return JS_TRUE;
}

#define PUSH_STATE_STACK(data)                                                \
    JS_BEGIN_MACRO                                                            \
        ++(data)->stateStackTop;                                              \
        if ((data)->stateStackTop == (data)->stateStackLimit &&               \
            !ReallocStateStack((data))) {                                     \
            return NULL;                                                      \
        }                                                                     \
    JS_END_MACRO

/*
 * Apply the current op against the given input to see if it's going to match
 * or fail. Return false if we don't get a match, true if we do. If updatecp is
 * true, then update the current state's cp. Always update startpc to the next
 * op.
 */
static REMatchState *
SimpleMatch(REGlobalData *gData, REMatchState *x, REOp op,
            jsbytecode **startpc, JSBool updatecp)
{
    REMatchState *result = NULL;
    uint32 matchCh;
    size_t parenIndex;
    size_t offset, length, index;
    jsbytecode *pc = *startpc;  /* pc has already been incremented past op */
    jschar *source;
    const jschar *startcp = x->cp;
    uint32 ch, point;
    uintN width;
    JSBool unicode = (gData->regexp->flags & JSREG_UNICODE) != 0;
    RECharSet *charSet;

    point = RegExpPoint(x->cp, gData->cpend, unicode, &width);
    switch (op) {
    case REOP_BOL:
        if (x->cp != gData->cpbegin) {
            if (!gData->cx->regExpStatics.multiline &&
                !(gData->regexp->flags & JSREG_MULTILINE)) {
                break;
            }
            if (!RE_IS_LINE_TERM(x->cp[-1]))
                break;
        }
        result = x;
        break;
    case REOP_EOL:
        if (x->cp != gData->cpend) {
            if (!gData->cx->regExpStatics.multiline &&
                !(gData->regexp->flags & JSREG_MULTILINE)) {
                break;
            }
            if (!RE_IS_LINE_TERM(*x->cp))
                break;
        }
        result = x;
        break;
    case REOP_WBDRY:
        if ((x->cp == gData->cpbegin || !(unicode ? UnicodeWord(x->cp[-1], (gData->regexp->flags & JSREG_FOLD) != 0) : JS_ISWORD(x->cp[-1]))) ^
            !(x->cp != gData->cpend && (unicode ? UnicodeWord(point, (gData->regexp->flags & JSREG_FOLD) != 0) : JS_ISWORD(*x->cp)))) {
            result = x;
        }
        break;
    case REOP_WNONBDRY:
        if ((x->cp == gData->cpbegin || !(unicode ? UnicodeWord(x->cp[-1], (gData->regexp->flags & JSREG_FOLD) != 0) : JS_ISWORD(x->cp[-1]))) ^
            (x->cp != gData->cpend && (unicode ? UnicodeWord(point, (gData->regexp->flags & JSREG_FOLD) != 0) : JS_ISWORD(*x->cp)))) {
            result = x;
        }
        break;
    case REOP_DOT:
        if (x->cp != gData->cpend && !RE_IS_LINE_TERM(*x->cp)) {
            result = x;
            result->cp += width;
        }
        break;
    case REOP_DIGIT:
        if (x->cp != gData->cpend && JS7_ISDEC(*x->cp)) {
            result = x;
            result->cp += width;
        }
        break;
    case REOP_NONDIGIT:
        if (x->cp != gData->cpend && !JS7_ISDEC(*x->cp)) {
            result = x;
            result->cp += width;
        }
        break;
    case REOP_ALNUM:
        if (x->cp != gData->cpend && (unicode ? UnicodeWord(point, (gData->regexp->flags & JSREG_FOLD) != 0) : JS_ISWORD(*x->cp))) {
            result = x;
            result->cp += width;
        }
        break;
    case REOP_NONALNUM:
        if (x->cp != gData->cpend && !(unicode ? UnicodeWord(point, (gData->regexp->flags & JSREG_FOLD) != 0) : JS_ISWORD(*x->cp))) {
            result = x;
            result->cp += width;
        }
        break;
    case REOP_SPACE:
        if (x->cp != gData->cpend && (point <= 0xFFFF && JS_ISSPACE((jschar)point))) {
            result = x;
            result->cp += width;
        }
        break;
    case REOP_NONSPACE:
        if (x->cp != gData->cpend && !(point <= 0xFFFF && JS_ISSPACE((jschar)point))) {
            result = x;
            result->cp += width;
        }
        break;
    case REOP_BACKREF:
        pc = ReadCompactIndex(pc, &parenIndex);
        JS_ASSERT(parenIndex < gData->regexp->parenCount);
        result = BackrefMatcher(gData, x, parenIndex);
        break;
    case REOP_FLAT:
        pc = ReadCompactIndex(pc, &offset);
        JS_ASSERT(offset < JSSTRING_LENGTH(gData->regexp->source));
        pc = ReadCompactIndex(pc, &length);
        JS_ASSERT(1 <= length);
        JS_ASSERT(length <= JSSTRING_LENGTH(gData->regexp->source) - offset);
        if (length <= (size_t)(gData->cpend - x->cp)) {
            source = JSSTRING_CHARS(gData->regexp->source) + offset;
            for (index = 0; index != length; index++) {
                if (source[index] != x->cp[index])
                    return NULL;
            }
            x->cp += length;
            result = x;
        }
        break;
    case REOP_FLAT1:
        matchCh = *pc++;
        if (x->cp != gData->cpend && point == matchCh) {
            result = x;
            result->cp += width;
        }
        break;
    case REOP_FLATi:
        pc = ReadCompactIndex(pc, &offset);
        JS_ASSERT(offset < JSSTRING_LENGTH(gData->regexp->source));
        pc = ReadCompactIndex(pc, &length);
        JS_ASSERT(1 <= length);
        JS_ASSERT(length <= JSSTRING_LENGTH(gData->regexp->source) - offset);
        source = JSSTRING_CHARS(gData->regexp->source);
        result = FlatNIMatcher(gData, x, source + offset, length);
        break;
    case REOP_FLAT1i:
        matchCh = *pc++;
        if (x->cp != gData->cpend && upcase(*x->cp) == upcase(matchCh)) {
            result = x;
            result->cp += width;
        }
        break;
    case REOP_UCFLAT1:
        matchCh = GET_ARG(pc);
        pc += ARG_LEN;
        if (x->cp != gData->cpend && point == matchCh) {
            result = x;
            result->cp += width;
        }
        break;
    case REOP_UCFLAT1i:
        matchCh = GET_ARG(pc);
        pc += ARG_LEN;
        if (x->cp != gData->cpend && upcase(*x->cp) == upcase(matchCh)) {
            result = x;
            result->cp += width;
        }
        break;
    case REOP_UCFLAT32:
        matchCh = ((uint32)GET_ARG(pc) << 16) | GET_ARG(pc+ARG_LEN);
        pc += 2*ARG_LEN;
        if (x->cp != gData->cpend &&
            ((gData->regexp->flags & JSREG_FOLD)
             ? js_UnicodeSimpleFold(point) == js_UnicodeSimpleFold(matchCh)
             : point == matchCh)) {
            result = x;
            result->cp += width;
        }
        break;
    case REOP_CLASS:
        pc = ReadCompactIndex(pc, &index);
        JS_ASSERT(index < gData->regexp->classCount);
        if (x->cp != gData->cpend) {
            charSet = &gData->regexp->classList[index];
            JS_ASSERT(charSet->converted);
            ch = unicode && (gData->regexp->flags & JSREG_FOLD)
                 ? js_UnicodeSimpleFold(point) : point;
            index = ch >> 3;
            if (charSet->length != 0 &&
                ch <= charSet->length &&
                (charSet->u.bits[index] & (1 << (ch & 0x7)))) {
                result = x;
                result->cp += width;
            }
        }
        break;
    case REOP_NCLASS:
        pc = ReadCompactIndex(pc, &index);
        JS_ASSERT(index < gData->regexp->classCount);
        if (x->cp != gData->cpend) {
            charSet = &gData->regexp->classList[index];
            JS_ASSERT(charSet->converted);
            ch = unicode && (gData->regexp->flags & JSREG_FOLD)
                 ? js_UnicodeSimpleFold(point) : point;
            index = ch >> 3;
            if (charSet->length == 0 ||
                ch > charSet->length ||
                !(charSet->u.bits[index] & (1 << (ch & 0x7)))) {
                result = x;
                result->cp += width;
            }
        }
        break;
    default:
        JS_ASSERT(JS_FALSE);
    }
    if (result) {
        if (!updatecp)
            x->cp = startcp;
        *startpc = pc;
        return result;
    }
    x->cp = startcp;
    return NULL;
}

static REMatchState *
ExecuteREBytecode(REGlobalData *gData, REMatchState *x)
{
    REMatchState *result = NULL;
    REBackTrackData *backTrackData;
    jsbytecode *nextpc, *testpc;
    REOp nextop;
    RECapture *cap;
    REProgState *curState=NULL;
    const jschar *startcp;
    size_t parenIndex, k;
    size_t parenSoFar = 0;

    jschar matchCh1, matchCh2;
    RECharSet *charSet;

    JSBranchCallback onbranch = gData->cx->branchCallback;
    uintN onbranchCalls = 0;
#define ONBRANCH_CALLS_MASK             127
#define CHECK_BRANCH()                                                         \
    JS_BEGIN_MACRO                                                             \
        if (onbranch &&                                                        \
            (++onbranchCalls & ONBRANCH_CALLS_MASK) == 0 &&                    \
            !(*onbranch)(gData->cx, NULL)) {                                   \
            gData->ok = JS_FALSE;                                              \
            return NULL;                                                       \
        }                                                                      \
    JS_END_MACRO

    JSBool anchor;
    jsbytecode *pc = gData->regexp->program;
    REOp op = (REOp) *pc++;

    /*
     * If the first node is a simple match, step the index into the string
     * until that match is made, or fail if it can't be found at all.
     */
    if (REOP_IS_SIMPLE(op)) {
        anchor = JS_FALSE;
        while (x->cp <= gData->cpend) {
            nextpc = pc;    /* reset back to start each time */
            result = SimpleMatch(gData, x, op, &nextpc, JS_TRUE);
            if (result) {
                anchor = JS_TRUE;
                x = result;
                pc = nextpc;    /* accept skip to next opcode */
                op = (REOp) *pc++;
                break;
            }
            if (gData->sticky)
                break;
            {
                uintN width;
                RegExpPoint(x->cp, gData->cpend,
                             (gData->regexp->flags & JSREG_UNICODE) != 0, &width);
                gData->skipped += width;
                x->cp += width;
            }
        }
        if (!anchor)
            return NULL;
    }

    for (;;) {
        if (REOP_IS_SIMPLE(op)) {
            result = SimpleMatch(gData, x, op, &pc, JS_TRUE);
        } else {
            curState = &gData->stateStack[gData->stateStackTop];
            switch (op) {
            case REOP_EMPTY:
                result = x;
                break;

            case REOP_ALTPREREQ2:
                nextpc = pc + GET_OFFSET(pc);   /* start of next op */
                pc += ARG_LEN;
                matchCh2 = GET_ARG(pc);
                pc += ARG_LEN;
                k = GET_ARG(pc);
                pc += ARG_LEN;

                if (x->cp != gData->cpend) {
                    if (*x->cp == matchCh2)
                        goto doAlt;

                    charSet = &gData->regexp->classList[k];
                    if (!charSet->converted && !ProcessCharSet(gData, charSet))
                        return NULL;
                    matchCh1 = *x->cp;
                    k = matchCh1 >> 3;
                    if ((charSet->length == 0 ||
                         matchCh1 > charSet->length ||
                         !(charSet->u.bits[k] & (1 << (matchCh1 & 0x7)))) ^
                        charSet->sense) {
                        goto doAlt;
                    }
                }
                result = NULL;
                break;

            case REOP_ALTPREREQ:
                nextpc = pc + GET_OFFSET(pc);   /* start of next op */
                pc += ARG_LEN;
                matchCh1 = GET_ARG(pc);
                pc += ARG_LEN;
                matchCh2 = GET_ARG(pc);
                pc += ARG_LEN;
                if (x->cp == gData->cpend ||
                    (*x->cp != matchCh1 && *x->cp != matchCh2)) {
                    result = NULL;
                    break;
                }
                /* else false thru... */

            case REOP_ALT:
            doAlt:
                nextpc = pc + GET_OFFSET(pc);   /* start of next alternate */
                pc += ARG_LEN;                  /* start of this alternate */
                curState->parenSoFar = parenSoFar;
                PUSH_STATE_STACK(gData);
                op = (REOp) *pc++;
                startcp = x->cp;
                if (REOP_IS_SIMPLE(op)) {
                    if (!SimpleMatch(gData, x, op, &pc, JS_TRUE)) {
                        op = (REOp) *nextpc++;
                        pc = nextpc;
                        continue;
                    }
                    result = x;
                    op = (REOp) *pc++;
                }
                nextop = (REOp) *nextpc++;
                if (!PushBackTrackState(gData, nextop, nextpc, x, startcp, 0, 0))
                    return NULL;
                continue;

            /*
             * Occurs at (successful) end of REOP_ALT,
             */
            case REOP_JUMP:
                if(gData->stateStackTop)
                  --gData->stateStackTop;
                pc += GET_OFFSET(pc);
                op = (REOp) *pc++;
                continue;

            /*
             * Occurs at last (successful) end of REOP_ALT,
             */
            case REOP_ENDALT:
                if(gData->stateStackTop)
                  --gData->stateStackTop;
                op = (REOp) *pc++;
                continue;

            case REOP_LPAREN:
                pc = ReadCompactIndex(pc, &parenIndex);
                JS_ASSERT(parenIndex < gData->regexp->parenCount);
                if (parenIndex + 1 > parenSoFar)
                    parenSoFar = parenIndex + 1;
                x->parens[parenIndex].index = x->cp - gData->cpbegin;
                x->parens[parenIndex].length = 0;
                op = (REOp) *pc++;
                continue;

            case REOP_RPAREN:
                pc = ReadCompactIndex(pc, &parenIndex);
                JS_ASSERT(parenIndex < gData->regexp->parenCount);
                cap = &x->parens[parenIndex];

                /*
                 * FIXME: https://bugzilla.mozilla.org/show_bug.cgi?id=346090
                 * This wallpaper prevents a case where we somehow took a step
                 * backward in input while minimally-matching an empty string.
                 */
                if (x->cp < gData->cpbegin + cap->index)
                    cap->index = -1;
                cap->length = x->cp - (gData->cpbegin + cap->index);
                op = (REOp) *pc++;
                continue;

            case REOP_ASSERT:
                nextpc = pc + GET_OFFSET(pc);  /* start of term after ASSERT */
                pc += ARG_LEN;                 /* start of ASSERT child */
                op = (REOp) *pc++;
                testpc = pc;
                if (REOP_IS_SIMPLE(op) &&
                    !SimpleMatch(gData, x, op, &testpc, JS_FALSE)) {
                    result = NULL;
                    break;
                }
                curState->u.assertion.top =
                    (char *)gData->backTrackSP - (char *)gData->backTrackStack;
                curState->u.assertion.sz = gData->cursz;
                curState->index = x->cp - gData->cpbegin;
                curState->parenSoFar = parenSoFar;
                PUSH_STATE_STACK(gData);
                if (!PushBackTrackState(gData, REOP_ASSERTTEST,
                                        nextpc, x, x->cp, 0, 0)) {
                    return NULL;
                }
                continue;

            case REOP_ASSERT_NOT:
                nextpc = pc + GET_OFFSET(pc);
                pc += ARG_LEN;
                op = (REOp) *pc++;
                testpc = pc;
                if (REOP_IS_SIMPLE(op) /* Note - fail to fail! */ &&
                    SimpleMatch(gData, x, op, &testpc, JS_FALSE) &&
                    *testpc == REOP_ASSERTNOTTEST) {
                    result = NULL;
                    break;
                }
                curState->u.assertion.top
                    = (char *)gData->backTrackSP -
                      (char *)gData->backTrackStack;
                curState->u.assertion.sz = gData->cursz;
                curState->index = x->cp - gData->cpbegin;
                curState->parenSoFar = parenSoFar;
                PUSH_STATE_STACK(gData);
                if (!PushBackTrackState(gData, REOP_ASSERTNOTTEST,
                                        nextpc, x, x->cp, 0, 0)) {
                    return NULL;
                }
                continue;

            case REOP_ASSERTTEST:
                if(gData->stateStackTop)
                  --gData->stateStackTop;
                --curState;
                x->cp = gData->cpbegin + curState->index;
                gData->backTrackSP =
                    (REBackTrackData *) ((char *)gData->backTrackStack +
                                         curState->u.assertion.top);
                gData->cursz = curState->u.assertion.sz;
                if (result)
                    result = x;
                break;

            case REOP_ASSERTNOTTEST:
                if(gData->stateStackTop)
                  --gData->stateStackTop;
                --curState;
                x->cp = gData->cpbegin + curState->index;
                gData->backTrackSP =
                    (REBackTrackData *) ((char *)gData->backTrackStack +
                                         curState->u.assertion.top);
                gData->cursz = curState->u.assertion.sz;
                result = (!result) ? x : NULL;
                break;

            case REOP_END:
                if (x)
                    return x;
                break;

            case REOP_STAR:
                curState->u.quantifier.min = 0;
                curState->u.quantifier.max = (uintN)-1;
                goto quantcommon;
            case REOP_PLUS:
                curState->u.quantifier.min = 1;
                curState->u.quantifier.max = (uintN)-1;
                goto quantcommon;
            case REOP_OPT:
                curState->u.quantifier.min = 0;
                curState->u.quantifier.max = 1;
                goto quantcommon;
            case REOP_QUANT:
                pc = ReadCompactIndex(pc, &k);
                curState->u.quantifier.min = k;
                pc = ReadCompactIndex(pc, &k);
                /* max is k - 1 to use one byte for (uintN)-1 sentinel. */
                curState->u.quantifier.max = k - 1;
                JS_ASSERT(curState->u.quantifier.min
                          <= curState->u.quantifier.max);
            quantcommon:
                if (curState->u.quantifier.max == 0) {
                    pc = pc + GET_OFFSET(pc);
                    op = (REOp) *pc++;
                    result = x;
                    continue;
                }
                /* Step over <next> */
                nextpc = pc + ARG_LEN;
                op = (REOp) *nextpc++;
                startcp = x->cp;
                if (REOP_IS_SIMPLE(op)) {
                    if (!SimpleMatch(gData, x, op, &nextpc, JS_TRUE)) {
                        if (curState->u.quantifier.min == 0)
                            result = x;
                        else
                            result = NULL;
                        pc = pc + GET_OFFSET(pc);
                        break;
                    }
                    op = (REOp) *nextpc++;
                    result = x;
                }
                curState->index = startcp - gData->cpbegin;
                curState->continue_op = REOP_REPEAT;
                curState->continue_pc = pc;
                curState->parenSoFar = parenSoFar;
                PUSH_STATE_STACK(gData);
                if (curState->u.quantifier.min == 0 &&
                    !PushBackTrackState(gData, REOP_REPEAT, pc, x, startcp,
                                        0, 0)) {
                    return NULL;
                }
                pc = nextpc;
                continue;

            case REOP_ENDCHILD: /* marks the end of a quantifier child */
                pc = curState[-1].continue_pc;
                op = curState[-1].continue_op;
                continue;

            case REOP_REPEAT:
                CHECK_BRANCH();
                --curState;
                do {
                    if(gData->stateStackTop)
                        --gData->stateStackTop;
                    if (!result) {
                        /* Failed, see if we have enough children. */
                        if (curState->u.quantifier.min == 0)
                            goto repeatDone;
                        goto break_switch;
                    }
                    if (curState->u.quantifier.min == 0 &&
                        x->cp == gData->cpbegin + curState->index) {
                        /* matched an empty string, that'll get us nowhere */
                        result = NULL;
                        goto break_switch;
                    }
                    if (curState->u.quantifier.min != 0)
                        curState->u.quantifier.min--;
                    if (curState->u.quantifier.max != (uintN) -1)
                        curState->u.quantifier.max--;
                    if (curState->u.quantifier.max == 0)
                        goto repeatDone;
                    nextpc = pc + ARG_LEN;
                    nextop = (REOp) *nextpc;
                    startcp = x->cp;
                    if (REOP_IS_SIMPLE(nextop)) {
                        nextpc++;
                        if (!SimpleMatch(gData, x, nextop, &nextpc, JS_TRUE)) {
                            if (curState->u.quantifier.min == 0)
                                goto repeatDone;
                            result = NULL;
                            goto break_switch;
                        }
                        result = x;
                    }
                    curState->index = startcp - gData->cpbegin;
                    PUSH_STATE_STACK(gData);
                    if (curState->u.quantifier.min == 0 &&
                        !PushBackTrackState(gData, REOP_REPEAT,
                                            pc, x, startcp,
                                            curState->parenSoFar,
                                            parenSoFar -
                                            curState->parenSoFar)) {
                        return NULL;
                    }
                } while (*nextpc == REOP_ENDCHILD);
                pc = nextpc;
                op = (REOp) *pc++;
                parenSoFar = curState->parenSoFar;
                continue;

            repeatDone:
                result = x;
                pc += GET_OFFSET(pc);
                goto break_switch;

            case REOP_MINIMALSTAR:
                curState->u.quantifier.min = 0;
                curState->u.quantifier.max = (uintN)-1;
                goto minimalquantcommon;
            case REOP_MINIMALPLUS:
                curState->u.quantifier.min = 1;
                curState->u.quantifier.max = (uintN)-1;
                goto minimalquantcommon;
            case REOP_MINIMALOPT:
                curState->u.quantifier.min = 0;
                curState->u.quantifier.max = 1;
                goto minimalquantcommon;
            case REOP_MINIMALQUANT:
                pc = ReadCompactIndex(pc, &k);
                curState->u.quantifier.min = k;
                pc = ReadCompactIndex(pc, &k);
                /* See REOP_QUANT comments about k - 1. */
                curState->u.quantifier.max = k - 1;
                JS_ASSERT(curState->u.quantifier.min
                          <= curState->u.quantifier.max);
            minimalquantcommon:
                curState->index = x->cp - gData->cpbegin;
                curState->parenSoFar = parenSoFar;
                PUSH_STATE_STACK(gData);
                if (curState->u.quantifier.min != 0) {
                    curState->continue_op = REOP_MINIMALREPEAT;
                    curState->continue_pc = pc;
                    /* step over <next> */
                    pc += OFFSET_LEN;
                    op = (REOp) *pc++;
                } else {
                    if (!PushBackTrackState(gData, REOP_MINIMALREPEAT,
                                            pc, x, x->cp, 0, 0)) {
                        return NULL;
                    }
                    if(gData->stateStackTop)
                      --gData->stateStackTop;
                    pc = pc + GET_OFFSET(pc);
                    op = (REOp) *pc++;
                }
                continue;

            case REOP_MINIMALREPEAT:
                CHECK_BRANCH();
                if(gData->stateStackTop)
                  --gData->stateStackTop;
                --curState;

                if (!result) {
                    /*
                     * Non-greedy failure - try to consume another child.
                     */
                    if (curState->u.quantifier.max == (uintN) -1 ||
                        curState->u.quantifier.max > 0) {
                        curState->index = x->cp - gData->cpbegin;
                        curState->continue_op = REOP_MINIMALREPEAT;
                        curState->continue_pc = pc;
                        pc += ARG_LEN;
                        for (k = curState->parenSoFar; k < parenSoFar; k++)
                            x->parens[k].index = -1;
                        PUSH_STATE_STACK(gData);
                        op = (REOp) *pc++;
                        continue;
                    }
                    /* Don't need to adjust pc since we're going to pop. */
                    break;
                }
                if (curState->u.quantifier.min == 0 &&
                    x->cp == gData->cpbegin + curState->index) {
                    /* Matched an empty string, that'll get us nowhere. */
                    result = NULL;
                    break;
                }
                if (curState->u.quantifier.min != 0)
                    curState->u.quantifier.min--;
                if (curState->u.quantifier.max != (uintN) -1)
                    curState->u.quantifier.max--;
                if (curState->u.quantifier.min != 0) {
                    curState->continue_op = REOP_MINIMALREPEAT;
                    curState->continue_pc = pc;
                    pc += ARG_LEN;
                    for (k = curState->parenSoFar; k < parenSoFar; k++)
                        x->parens[k].index = -1;
                    curState->index = x->cp - gData->cpbegin;
                    PUSH_STATE_STACK(gData);
                    op = (REOp) *pc++;
                    continue;
                }
                curState->index = x->cp - gData->cpbegin;
                curState->parenSoFar = parenSoFar;
                PUSH_STATE_STACK(gData);
                if (!PushBackTrackState(gData, REOP_MINIMALREPEAT,
                                        pc, x, x->cp,
                                        curState->parenSoFar,
                                        parenSoFar - curState->parenSoFar)) {
                    return NULL;
                }
                if(gData->stateStackTop)
                    --gData->stateStackTop;
                pc = pc + GET_OFFSET(pc);
                op = (REOp) *pc++;
                continue;

            default:
                JS_ASSERT(JS_FALSE);
                result = NULL;
            }
        break_switch:;
        }

        /*
         *  If the match failed and there's a backtrack option, take it.
         *  Otherwise this is a complete and utter failure.
         */
        if (!result) {
            if (gData->cursz == 0)
                return NULL;
            backTrackData = gData->backTrackSP;
            gData->cursz = backTrackData->sz;
            gData->backTrackSP =
                (REBackTrackData *) ((char *)backTrackData - backTrackData->sz);
            x->cp = backTrackData->cp;
            pc = backTrackData->backtrack_pc;
            op = backTrackData->backtrack_op;
            gData->stateStackTop = backTrackData->saveStateStackTop;

            JS_ASSERT(gData->stateStackTop);

            memcpy(gData->stateStack, backTrackData + 1,
                   sizeof(REProgState) * backTrackData->saveStateStackTop);
            curState = &gData->stateStack[gData->stateStackTop - 1];

            if (backTrackData->parenCount) {
                memcpy(&x->parens[backTrackData->parenIndex],
                       (char *)(backTrackData + 1) +
                       sizeof(REProgState) * backTrackData->saveStateStackTop,
                       sizeof(RECapture) * backTrackData->parenCount);
                parenSoFar = backTrackData->parenIndex + backTrackData->parenCount;
            } else {
                for (k = curState->parenSoFar; k < parenSoFar; k++)
                    x->parens[k].index = -1;
                parenSoFar = curState->parenSoFar;
            }
            continue;
        }
        x = result;

        /*
         *  Continue with the expression.
         */
        op = (REOp)*pc++;
    }
    return NULL;
}

static REMatchState *
MatchRegExp(REGlobalData *gData, REMatchState *x)
{
    REMatchState *result;
    const jschar *cp = x->cp;
    const jschar *cp2;
    uintN j;

    /*
     * Have to include the position beyond the last character
     * in order to detect end-of-input/line condition.
     */
    for (cp2 = cp; cp2 <= gData->cpend;) {
        gData->skipped = cp2 - cp;
        x->cp = cp2;
        for (j = 0; j < gData->regexp->parenCount; j++)
            x->parens[j].index = -1;
        result = ExecuteREBytecode(gData, x);
        if (!gData->ok || result || gData->sticky)
            return result;
        gData->backTrackSP = gData->backTrackStack;
        gData->cursz = 0;
        gData->stateStackTop = 0;
        cp2 = cp + gData->skipped;
        {
            uintN width;
            RegExpPoint(cp2, gData->cpend,
                         (gData->regexp->flags & JSREG_UNICODE) != 0, &width);
            cp2 += width;
        }
    }
    return NULL;
}


static REMatchState *
InitMatch(JSContext *cx, REGlobalData *gData, JSRegExp *re)
{
    REMatchState *result;
    uintN i;

    gData->backTrackStackSize = INITIAL_BACKTRACK;
    JS_ARENA_ALLOCATE_CAST(gData->backTrackStack, REBackTrackData *,
                           &gData->pool,
                           INITIAL_BACKTRACK);
    if (!gData->backTrackStack)
        goto bad;

    gData->backTrackSP = gData->backTrackStack;
    gData->cursz = 0;

    gData->stateStackLimit = INITIAL_STATESTACK;
    JS_ARENA_ALLOCATE_CAST(gData->stateStack, REProgState *,
                           &gData->pool,
                           sizeof(REProgState) * INITIAL_STATESTACK);
    if (!gData->stateStack)
        goto bad;

    gData->stateStackTop = 0;
    gData->cx = cx;
    gData->regexp = re;
    gData->ok = JS_TRUE;

    JS_ARENA_ALLOCATE_CAST(result, REMatchState *,
                           &gData->pool,
                           offsetof(REMatchState, parens)
                           + re->parenCount * sizeof(RECapture));
    if (!result)
        goto bad;

    for (i = 0; i < re->classCount; i++) {
        if (!re->classList[i].converted &&
            !ProcessCharSet(gData, &re->classList[i])) {
            return NULL;
        }
    }

    return result;

bad:
    JS_ReportOutOfMemory(cx);
    gData->ok = JS_FALSE;
    return NULL;
}

void
js_RegExpStatics_clear(JSContext *cx, JSRegExpStatics *res)
{
    res->input = NULL;
    res->pendingInput = NULL;
    res->multiline = JS_FALSE;
    res->parenCount = 0;
    res->lastMatch = res->lastParen = js_EmptySubString;
    res->leftContext = res->rightContext = js_EmptySubString;
    if (res->moreParens) {
        JS_free(cx, res->moreParens);
        res->moreParens = NULL;
    }
}

static JSBool
ExecuteRegExp(JSContext *cx, JSRegExp *re, JSString *str, size_t *indexp,
              JSBool test, JSBool sticky, JSObject *global, jsval *rval)
{
    REGlobalData gData;
    REMatchState *x, *result;

    const jschar *cp, *ep;
    size_t i, length, start;
    JSSubString *morepar;
    JSBool ok;
    JSRegExpStatics *res = &cx->regExpStatics;
    ptrdiff_t matchlen;
    uintN num, morenum;
    JSString *parstr, *matchstr;
    JSObject *obj, *proto;

    RECapture *parsub = NULL;

    /*
     * It's safe to load from cp because JSStrings have a zero at the end,
     * and we never let cp get beyond cpend.
     */
    start = *indexp;
    length = JSSTRING_LENGTH(str);
    if (start > length)
        start = length;
    cp = JSSTRING_CHARS(str);
    if ((re->flags & JSREG_UNICODE) && start && start < length &&
        cp[start] >= 0xDC00 && cp[start] <= 0xDFFF &&
        cp[start-1] >= 0xD800 && cp[start-1] <= 0xDBFF) --start;
    gData.cpbegin = cp;
    gData.cpend = cp + length;
    cp += start;
    gData.start = start;
    gData.skipped = 0;
    gData.sticky = sticky;

    JS_InitArenaPool(&gData.pool, "RegExpPool", 8096, 4);
    x = InitMatch(cx, &gData, re);
    if (!x) {
        ok = JS_FALSE;
        goto out;
    }
    x->cp = cp;

    /*
     * Call the recursive matcher to do the real work.  Return null on mismatch
     * whether testing or not.  On match, return an extended Array object.
     */
    result = MatchRegExp(&gData, x);
    ok = gData.ok;
    if (!ok)
        goto out;
    if (!result) {
        *rval = JSVAL_NULL;
        goto out;
    }
    cp = result->cp;
    i = cp - gData.cpbegin;
    *indexp = i;
    matchlen = i - (start + gData.skipped);
    ep = cp;
    cp -= matchlen;

    if (test) {
        /*
         * Testing for a match and updating cx->regExpStatics: don't allocate
         * an array object, do return true.
         */
        *rval = JSVAL_TRUE;

        /* Avoid warning.  (gcc doesn't detect that obj is needed iff !test); */
        obj = NULL;
    } else {
        /*
         * The array returned on match has element 0 bound to the matched
         * string, elements 1 through state.parenCount bound to the paren
         * matches, an index property telling the length of the left context,
         * and an input property referring to the input string.
         */
        if (global) {
            proto = js_BuiltinPrototype(cx, global, JSProto_Array);
            if (!proto) { ok = JS_FALSE; goto out; }
            obj = js_NewArrayObjectWithProto(cx, 0, NULL, proto, global);
        } else obj = js_NewArrayObject(cx, 0, NULL);
        if (!obj) {
            ok = JS_FALSE;
            goto out;
        }
        *rval = OBJECT_TO_JSVAL(obj);

#define DEFVAL(val, id) {                                                     \
    ok = js_DefineProperty(cx, obj, id, val,                                  \
                           JS_PropertyStub, JS_PropertyStub,                  \
                           JSPROP_ENUMERATE, NULL);                           \
    if (!ok) {                                                                \
        cx->weakRoots.newborn[GCX_OBJECT] = NULL;                             \
        cx->weakRoots.newborn[GCX_STRING] = NULL;                             \
        goto out;                                                             \
    }                                                                         \
}

        matchstr = js_NewStringCopyN(cx, cp, matchlen, 0);
        if (!matchstr) {
            cx->weakRoots.newborn[GCX_OBJECT] = NULL;
            ok = JS_FALSE;
            goto out;
        }
        DEFVAL(STRING_TO_JSVAL(matchstr), INT_TO_JSID(0));
    }

    res->pendingInput = res->input = str;
    res->parenCount = re->parenCount;
    if (re->parenCount == 0) {
        res->lastParen = js_EmptySubString;
    } else {
        for (num = 0; num < re->parenCount; num++) {
            parsub = &result->parens[num];
            if (num < 9) {
                if (parsub->index == -1) {
                    res->parens[num].chars = NULL;
                    res->parens[num].length = 0;
                } else {
                    res->parens[num].chars = gData.cpbegin + parsub->index;
                    res->parens[num].length = parsub->length;
                }
            } else {
                morenum = num - 9;
                morepar = res->moreParens;
                if (!morepar) {
                    res->moreLength = 10;
                    morepar = (JSSubString*)
                        JS_malloc(cx, 10 * sizeof(JSSubString));
                } else if (morenum >= res->moreLength) {
                    res->moreLength += 10;
                    morepar = (JSSubString*)
                        JS_realloc(cx, morepar,
                                   res->moreLength * sizeof(JSSubString));
                }
                if (!morepar) {
                    cx->weakRoots.newborn[GCX_OBJECT] = NULL;
                    cx->weakRoots.newborn[GCX_STRING] = NULL;
                    ok = JS_FALSE;
                    goto out;
                }
                res->moreParens = morepar;
                if (parsub->index == -1) {
                    morepar[morenum].chars = NULL;
                    morepar[morenum].length = 0;
                } else {
                    morepar[morenum].chars = gData.cpbegin + parsub->index;
                    morepar[morenum].length = parsub->length;
                }
            }
            if (test)
                continue;
            if (parsub->index == -1) {
                ok = js_DefineProperty(cx, obj, INT_TO_JSID(num + 1),
                                       JSVAL_VOID, NULL, NULL,
                                       JSPROP_ENUMERATE, NULL);
            } else {
                parstr = js_NewStringCopyN(cx, gData.cpbegin + parsub->index,
                                           parsub->length, 0);
                if (!parstr) {
                    cx->weakRoots.newborn[GCX_OBJECT] = NULL;
                    cx->weakRoots.newborn[GCX_STRING] = NULL;
                    ok = JS_FALSE;
                    goto out;
                }
                ok = js_DefineProperty(cx, obj, INT_TO_JSID(num + 1),
                                       STRING_TO_JSVAL(parstr), NULL, NULL,
                                       JSPROP_ENUMERATE, NULL);
            }
            if (!ok) {
                cx->weakRoots.newborn[GCX_OBJECT] = NULL;
                cx->weakRoots.newborn[GCX_STRING] = NULL;
                goto out;
            }
        }
        if (parsub->index == -1) {
            res->lastParen = js_EmptySubString;
        } else {
            res->lastParen.chars = gData.cpbegin + parsub->index;
            res->lastParen.length = parsub->length;
        }
    }

    if (!test) {
        /*
         * Define the index and input properties last for better for/in loop
         * order (so they come after the elements).
         */
        DEFVAL(INT_TO_JSVAL(start + gData.skipped),
               ATOM_TO_JSID(cx->runtime->atomState.indexAtom));
        DEFVAL(STRING_TO_JSVAL(str),
               ATOM_TO_JSID(cx->runtime->atomState.inputAtom));
    }

#undef DEFVAL

    res->lastMatch.chars = cp;
    res->lastMatch.length = matchlen;

    /*
     * For JS1.3 and ECMAv2, emulate Perl5 exactly:
     *
     * js1.3        "hi", "hi there"            "hihitherehi therebye"
     */
    res->leftContext.chars = JSSTRING_CHARS(str);
    res->leftContext.length = start + gData.skipped;
    res->rightContext.chars = ep;
    res->rightContext.length = gData.cpend - ep;

out:
    if (!ok)
        js_RegExpStatics_clear(cx,res);
    JS_FinishArenaPool(&gData.pool);
    return ok;
}

JSBool
js_ExecuteRegExp(JSContext *cx, JSRegExp *re, JSString *str, size_t *indexp,
                 JSBool test, jsval *rval)
{
    return ExecuteRegExp(cx, re, str, indexp, test, (re->flags & JSREG_STICKY) != 0, NULL, rval);
}

/************************************************************************/

enum regexp_tinyid {
    REGEXP_SOURCE       = -1,
    REGEXP_GLOBAL       = -2,
    REGEXP_IGNORE_CASE  = -3,
    REGEXP_LAST_INDEX   = -4,
    REGEXP_MULTILINE    = -5,
    REGEXP_STICKY       = -6,
    REGEXP_UNICODE      = -7
};

#define REGEXP_PROP_ATTRS (JSPROP_PERMANENT|JSPROP_SHARED)

static JSPropertySpec regexp_props[] = {
    {"source",     REGEXP_SOURCE,      REGEXP_PROP_ATTRS | JSPROP_READONLY,0,0},
    {"global",     REGEXP_GLOBAL,      REGEXP_PROP_ATTRS | JSPROP_READONLY,0,0},
    {"ignoreCase", REGEXP_IGNORE_CASE, REGEXP_PROP_ATTRS | JSPROP_READONLY,0,0},
    {"lastIndex",  REGEXP_LAST_INDEX,  REGEXP_PROP_ATTRS,0,0},
    {"multiline",  REGEXP_MULTILINE,   REGEXP_PROP_ATTRS | JSPROP_READONLY,0,0},
    {"unicode",    REGEXP_UNICODE,     REGEXP_PROP_ATTRS | JSPROP_READONLY,0,0},
    {0,0,0,0,0}
};

static JSBool
regexp_getProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
    jsint slot;
    JSRegExp *re;

    if (!JSVAL_IS_INT(id))
        return JS_TRUE;
    slot = JSVAL_TO_INT(id);
    if (slot == REGEXP_LAST_INDEX)
        return JS_GetReservedSlot(cx, obj, 0, vp);

    JS_LOCK_OBJ(cx, obj);
    re = (JSRegExp *) JS_GetInstancePrivate(cx, obj, &js_RegExpClass, NULL);
    if (re) {
        switch (slot) {
          case REGEXP_SOURCE:
            *vp = STRING_TO_JSVAL(re->source);
            break;
          case REGEXP_GLOBAL:
            *vp = BOOLEAN_TO_JSVAL((re->flags & JSREG_GLOB) != 0);
            break;
          case REGEXP_IGNORE_CASE:
            *vp = BOOLEAN_TO_JSVAL((re->flags & JSREG_FOLD) != 0);
            break;
          case REGEXP_MULTILINE:
            *vp = BOOLEAN_TO_JSVAL((re->flags & JSREG_MULTILINE) != 0);
            break;
          case REGEXP_UNICODE:
            *vp = BOOLEAN_TO_JSVAL((re->flags & JSREG_UNICODE) != 0);
            break;
        }
    }
    JS_UNLOCK_OBJ(cx, obj);
    return JS_TRUE;
}

static JSBool
regexp_setProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
    /* ES5 stores lastIndex verbatim; exec performs ToInteger when using it. */
    return !JSVAL_IS_INT(id) || JSVAL_TO_INT(id) != REGEXP_LAST_INDEX ||
           JS_SetReservedSlot(cx, obj, 0, *vp);
}

/* An ES2015 RegExp prototype is an ordinary object, without a matcher. Its
 * private class keeps the RegExp intrinsic cache key without giving it the
 * legacy RegExp instance hooks or private-data layout. */
static JSClass regexpPrototypeClass = {
    "RegExp", JSCLASS_HAS_CACHED_PROTO(JSProto_RegExp),
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub,
    JSCLASS_NO_OPTIONAL_MEMBERS
};

static JSBool
RegExpReceiverError(JSContext *cx, const char *name)
{
    JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_INCOMPATIBLE_PROTO,
                         "RegExp", name, "receiver");
    return JS_FALSE;
}

static JSBool
EscapeRegExpSource(JSContext *cx, JSString *source, jsval *rval)
{
    size_t i, j, amount, length = JSSTRING_LENGTH(source);
    uintN pass;
    JSBool escaped;
    const char *replacement, *p;
    jschar ch, *chars = NULL;
    JSString *str;
    if (!length) {
        str = JS_NewStringCopyZ(cx, "(?:)");
        if (!str) return JS_FALSE;
        *rval = STRING_TO_JSVAL(str);
        return JS_TRUE;
    }
    for (pass = 0; pass < 2; ++pass) {
        j = 0; escaped = JS_FALSE;
        for (i = 0; i < length; ++i) {
            ch = JSSTRING_CHARS(source)[i];
            replacement = ch == '\n' ? "\\n" : ch == '\r' ? "\\r" :
                          ch == 0x2028 ? "\\u2028" : ch == 0x2029 ? "\\u2029" :
                          ch == '/' && !escaped ? "\\/" : NULL;
            if (replacement && escaped) ++replacement;
            amount = replacement ? strlen(replacement) : 1;
            if (amount > JSSTRING_LENGTH_MASK - j) {
                JS_ReportOutOfMemory(cx); return JS_FALSE;
            }
            if (!pass) j += amount;
            else if (replacement) {
                for (p = replacement; *p; ++p) chars[j++] = (jschar)*p;
            } else chars[j++] = ch;
            escaped = ch == '\\' ? !escaped : JS_FALSE;
        }
        if (!pass) {
            if (j >= ((size_t)-1) / sizeof(jschar)) {
                JS_ReportOutOfMemory(cx); return JS_FALSE;
            }
            chars = (jschar *)JS_malloc(cx, (j + 1) * sizeof(jschar));
            if (!chars) return JS_FALSE;
        }
    }
    chars[j] = 0;
    str = js_NewString(cx, chars, j, 0);
    if (!str) { JS_free(cx, chars); return JS_FALSE; }
    *rval = STRING_TO_JSVAL(str);
    return JS_TRUE;
}

static JSBool
ModernRegExpField(JSContext *cx, jsval *argv, jsint field, jsval *rval)
{
    JSObject *receiver;
    JSRegExp *re;
    uintN flags;
    if (JSVAL_IS_PRIMITIVE(argv[-1])) return RegExpReceiverError(cx, "accessor");
    receiver = JSVAL_TO_OBJECT(argv[-1]);
    /* ES2015 has no special case for the ordinary RegExp prototype.
     * Its missing matcher slots must throw, unlike later editions. */
    if (OBJ_GET_CLASS(cx, receiver) != &js_RegExpClass)
        return RegExpReceiverError(cx, "accessor");
    JS_LOCK_OBJ(cx, receiver);
    re = (JSRegExp *)JS_GetPrivate(cx, receiver);
    if (!re) {
        JS_UNLOCK_OBJ(cx, receiver);
        return RegExpReceiverError(cx, "accessor");
    }
    flags = re->flags;
    if (field == REGEXP_SOURCE) *rval = STRING_TO_JSVAL(re->source);
    JS_UNLOCK_OBJ(cx, receiver);
    if (field == REGEXP_SOURCE)
        return EscapeRegExpSource(cx, JSVAL_TO_STRING(*rval), rval);
    *rval = BOOLEAN_TO_JSVAL((flags & (field == REGEXP_GLOBAL ? JSREG_GLOB :
                         field == REGEXP_IGNORE_CASE ? JSREG_FOLD :
                         field == REGEXP_UNICODE ? JSREG_UNICODE :
                         field == REGEXP_STICKY ? JSREG_STICKY : JSREG_MULTILINE)) != 0);
    return JS_TRUE;
}
#define REGEXP_FIELD_GETTER(name, field) \
static JSBool name(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) \
{ return ModernRegExpField(cx, argv, field, rval); }
REGEXP_FIELD_GETTER(regexp_sourceGetter, REGEXP_SOURCE)
REGEXP_FIELD_GETTER(regexp_globalGetter, REGEXP_GLOBAL)
REGEXP_FIELD_GETTER(regexp_ignoreCaseGetter, REGEXP_IGNORE_CASE)
REGEXP_FIELD_GETTER(regexp_multilineGetter, REGEXP_MULTILINE)
REGEXP_FIELD_GETTER(regexp_stickyGetter, REGEXP_STICKY)
REGEXP_FIELD_GETTER(regexp_unicodeGetter, REGEXP_UNICODE)
#undef REGEXP_FIELD_GETTER

static JSBool
regexp_flagsGetter(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    static const char *names[] = {"global", "ignoreCase", "multiline", "unicode", "sticky"};
    static const char letters[] = "gimuy";
    jschar flags[5];
    uintN i, length = 0;
    JSBool boolean, ok = JS_FALSE;
    JSTempValueRooter root;
    JSString *str;
    if (JSVAL_IS_PRIMITIVE(argv[-1])) return RegExpReceiverError(cx, "flags");
    obj = JSVAL_TO_OBJECT(argv[-1]);
    JS_PUSH_SINGLE_TEMP_ROOT(cx, JSVAL_VOID, &root);
    for (i = 0; i < 5; ++i) {
        if (!JS_GetProperty(cx, obj, names[i], &root.u.value) ||
            !JS_ValueToBoolean(cx, root.u.value, &boolean)) goto out;
        if (boolean) flags[length++] = letters[i];
    }
    str = JS_NewUCStringCopyN(cx, flags, length);
    if (!str) goto out;
    *rval = STRING_TO_JSVAL(str); ok = JS_TRUE;
  out:
    JS_POP_TEMP_ROOT(cx, &root);
    return ok;
}

static JSBool
ModernRegExpString(JSContext *cx, jsval *argv, jsval *rval)
{
    JSObject *obj;
    jsval roots[2] = {JSVAL_VOID, JSVAL_VOID};
    JSTempValueRooter root;
    JSString *source, *flags, *str;
    jschar *chars;
    size_t sourceLength, flagsLength, length;
    JSBool ok = JS_FALSE;
    if (JSVAL_IS_PRIMITIVE(argv[-1])) return RegExpReceiverError(cx, "toString");
    obj = JSVAL_TO_OBJECT(argv[-1]);
    JS_PUSH_TEMP_ROOT(cx, 2, roots, &root);
    if (!JS_GetProperty(cx, obj, "source", &roots[0])) goto out;
    source = js_ValueToString(cx, roots[0]);
    if (!source) goto out;
    roots[0] = STRING_TO_JSVAL(source);
    if (!JS_GetProperty(cx, obj, "flags", &roots[1])) goto out;
    flags = js_ValueToString(cx, roots[1]);
    if (!flags) goto out;
    roots[1] = STRING_TO_JSVAL(flags);
    sourceLength = JSSTRING_LENGTH(source); flagsLength = JSSTRING_LENGTH(flags);
    if (sourceLength > JSSTRING_LENGTH_MASK - 2 ||
        flagsLength > JSSTRING_LENGTH_MASK - 2 - sourceLength) {
        JS_ReportOutOfMemory(cx); goto out;
    }
    length = sourceLength + flagsLength + 2;
    chars = (jschar *)JS_malloc(cx, (length + 1) * sizeof(jschar));
    if (!chars) goto out;
    chars[0] = '/';
    js_strncpy(chars + 1, JSSTRING_CHARS(source), sourceLength);
    chars[sourceLength + 1] = '/';
    js_strncpy(chars + sourceLength + 2, JSSTRING_CHARS(flags), flagsLength);
    chars[length] = 0;
    str = js_NewString(cx, chars, length, 0);
    if (!str) { JS_free(cx, chars); goto out; }
    *rval = STRING_TO_JSVAL(str); ok = JS_TRUE;
  out:
    JS_POP_TEMP_ROOT(cx, &root);
    return ok;
}

static JSBool
InitRegExpAccessors(JSContext *cx, JSObject *global, JSObject *proto)
{
    static struct { const char *name; JSNative native; } getters[] = {
        {"source", regexp_sourceGetter}, {"global", regexp_globalGetter},
        {"ignoreCase", regexp_ignoreCaseGetter}, {"multiline", regexp_multilineGetter},
        {"sticky", regexp_stickyGetter}, {"flags", regexp_flagsGetter},
        {"unicode", regexp_unicodeGetter}
    };
    uintN i;
    char name[32];
    JSFunction *fun;
    JSTempValueRooter root;
    JSBool ok;
    for (i = 0; i < sizeof(getters) / sizeof(getters[0]); ++i) {
        JS_snprintf(name, sizeof(name), "get %s", getters[i].name);
        fun = JS_NewFunction(cx, getters[i].native, 0, JSFUN_STRICT | JSFUN_NO_CONSTRUCT,
                             global, name);
        if (!fun) return JS_FALSE;
        JS_PUSH_TEMP_ROOT_OBJECT(cx, fun->object, &root);
        ok = JS_DefineProperty(cx, proto, getters[i].name, JSVAL_VOID,
                               (JSPropertyOp)fun->object, NULL, JSPROP_GETTER | JSPROP_SHARED);
        JS_POP_TEMP_ROOT(cx, &root);
        if (!ok) return JS_FALSE;
    }
    return JS_TRUE;
}

/* Different private-slot layouts prevent map sharing with the modern
 * prototype, so provide its parent explicitly for native-created instances. */
static JSObject *
NewRegExpInstance(JSContext *cx, JSObject *parent)
{
    JSObject *proto;
    if (!js_GetClassPrototype(cx, parent, INT_TO_JSID(JSProto_RegExp), &proto))
        return NULL;
    if (!parent && proto && OBJ_GET_CLASS(cx, proto) == &regexpPrototypeClass)
        parent = OBJ_GET_PARENT(cx, proto);
    return js_NewObject(cx, &js_RegExpClass, proto, parent);
}

/*
 * RegExp class static properties and their Perl counterparts:
 *
 *  RegExp.input                $_
 *  RegExp.multiline            $*
 *  RegExp.lastMatch            $&
 *  RegExp.lastParen            $+
 *  RegExp.leftContext          $`
 *  RegExp.rightContext         $'
 */
enum regexp_static_tinyid {
    REGEXP_STATIC_INPUT         = -1,
    REGEXP_STATIC_MULTILINE     = -2,
    REGEXP_STATIC_LAST_MATCH    = -3,
    REGEXP_STATIC_LAST_PAREN    = -4,
    REGEXP_STATIC_LEFT_CONTEXT  = -5,
    REGEXP_STATIC_RIGHT_CONTEXT = -6
};

JSBool
js_InitRegExpStatics(JSContext *cx, JSRegExpStatics *res)
{
    JSBool in, pd;
    JS_ClearRegExpStatics(cx);
    in = js_AddRoot(cx, &res->input, "res->input");
    pd = js_AddRoot(cx, &res->pendingInput, "res->input");
    return in && pd;
}

void
js_FreeRegExpStatics(JSContext *cx, JSRegExpStatics *res)
{
    if (res->moreParens) {
        JS_free(cx, res->moreParens);
        res->moreParens = NULL;
    }
    js_RemoveRoot(cx->runtime, &res->input);
    js_RemoveRoot(cx->runtime, &res->pendingInput);
}

static JSBool
regexp_static_getProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
    jsint slot;
    JSRegExpStatics *res;
    JSString *str;
    JSSubString *sub;

    res = &cx->regExpStatics;
    if (!JSVAL_IS_INT(id))
        return JS_TRUE;
    slot = JSVAL_TO_INT(id);
    switch (slot) {
      case REGEXP_STATIC_INPUT:
        *vp = res->pendingInput ? STRING_TO_JSVAL(res->pendingInput)
                                : JS_GetEmptyStringValue(cx);
        return JS_TRUE;
      case REGEXP_STATIC_MULTILINE:
        *vp = BOOLEAN_TO_JSVAL(res->multiline);
        return JS_TRUE;
      case REGEXP_STATIC_LAST_MATCH:
        sub = &res->lastMatch;
        break;
      case REGEXP_STATIC_LAST_PAREN:
        sub = &res->lastParen;
        break;
      case REGEXP_STATIC_LEFT_CONTEXT:
        sub = &res->leftContext;
        break;
      case REGEXP_STATIC_RIGHT_CONTEXT:
        sub = &res->rightContext;
        break;
      default:
        sub = REGEXP_PAREN_SUBSTRING(res, slot);
        break;
    }
    str = js_NewStringCopyN(cx, sub->chars, sub->length, 0);
    if (!str)
        return JS_FALSE;
    *vp = STRING_TO_JSVAL(str);
    return JS_TRUE;
}

JSBool
js_IsRegExpStaticPropertyHook(JSPropertyOp getter)
{
    return getter == regexp_static_getProperty;
}

static JSBool
regexp_static_setProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
    JSRegExpStatics *res;

    if (!JSVAL_IS_INT(id))
        return JS_TRUE;
    res = &cx->regExpStatics;
    /* XXX use if-else rather than switch to keep MSVC1.52 from crashing */
    if (JSVAL_TO_INT(id) == REGEXP_STATIC_INPUT) {
        if (!JSVAL_IS_STRING(*vp) &&
            !JS_ConvertValue(cx, *vp, JSTYPE_STRING, vp)) {
            return JS_FALSE;
        }
        res->pendingInput = JSVAL_TO_STRING(*vp);
    } else if (JSVAL_TO_INT(id) == REGEXP_STATIC_MULTILINE) {
        if (!JSVAL_IS_BOOLEAN(*vp) &&
            !JS_ConvertValue(cx, *vp, JSTYPE_BOOLEAN, vp)) {
            return JS_FALSE;
        }
        res->multiline = JSVAL_TO_BOOLEAN(*vp);
    }
    return JS_TRUE;
}

static JSPropertySpec regexp_static_props[] = {
    {"input",
     REGEXP_STATIC_INPUT,
     JSPROP_ENUMERATE|JSPROP_SHARED,
     regexp_static_getProperty,    regexp_static_setProperty},
    {"multiline",
     REGEXP_STATIC_MULTILINE,
     JSPROP_ENUMERATE|JSPROP_SHARED,
     regexp_static_getProperty,    regexp_static_setProperty},
    {"lastMatch",
     REGEXP_STATIC_LAST_MATCH,
     JSPROP_ENUMERATE|JSPROP_READONLY|JSPROP_SHARED,
     regexp_static_getProperty,    regexp_static_getProperty},
    {"lastParen",
     REGEXP_STATIC_LAST_PAREN,
     JSPROP_ENUMERATE|JSPROP_READONLY|JSPROP_SHARED,
     regexp_static_getProperty,    regexp_static_getProperty},
    {"leftContext",
     REGEXP_STATIC_LEFT_CONTEXT,
     JSPROP_ENUMERATE|JSPROP_READONLY|JSPROP_SHARED,
     regexp_static_getProperty,    regexp_static_getProperty},
    {"rightContext",
     REGEXP_STATIC_RIGHT_CONTEXT,
     JSPROP_ENUMERATE|JSPROP_READONLY|JSPROP_SHARED,
     regexp_static_getProperty,    regexp_static_getProperty},

    /* XXX should have block scope and local $1, etc. */
    {"$1", 0, JSPROP_ENUMERATE|JSPROP_READONLY|JSPROP_SHARED,
     regexp_static_getProperty,    regexp_static_getProperty},
    {"$2", 1, JSPROP_ENUMERATE|JSPROP_READONLY|JSPROP_SHARED,
     regexp_static_getProperty,    regexp_static_getProperty},
    {"$3", 2, JSPROP_ENUMERATE|JSPROP_READONLY|JSPROP_SHARED,
     regexp_static_getProperty,    regexp_static_getProperty},
    {"$4", 3, JSPROP_ENUMERATE|JSPROP_READONLY|JSPROP_SHARED,
     regexp_static_getProperty,    regexp_static_getProperty},
    {"$5", 4, JSPROP_ENUMERATE|JSPROP_READONLY|JSPROP_SHARED,
     regexp_static_getProperty,    regexp_static_getProperty},
    {"$6", 5, JSPROP_ENUMERATE|JSPROP_READONLY|JSPROP_SHARED,
     regexp_static_getProperty,    regexp_static_getProperty},
    {"$7", 6, JSPROP_ENUMERATE|JSPROP_READONLY|JSPROP_SHARED,
     regexp_static_getProperty,    regexp_static_getProperty},
    {"$8", 7, JSPROP_ENUMERATE|JSPROP_READONLY|JSPROP_SHARED,
     regexp_static_getProperty,    regexp_static_getProperty},
    {"$9", 8, JSPROP_ENUMERATE|JSPROP_READONLY|JSPROP_SHARED,
     regexp_static_getProperty,    regexp_static_getProperty},

    {0,0,0,0,0}
};

static void
regexp_finalize(JSContext *cx, JSObject *obj)
{
    JSRegExp *re;

    re = (JSRegExp *) JS_GetPrivate(cx, obj);
    if (!re)
        return;
    js_DestroyRegExp(cx, re);
}

/* Forward static prototype. */
static JSBool
regexp_exec(JSContext *cx, JSObject *obj, uintN argc, jsval *argv,
            jsval *rval);

static JSBool
regexp_call(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    return regexp_exec(cx, JSVAL_TO_OBJECT(argv[-2]), argc, argv, rval);
}

#if JS_HAS_XDR

#include "jsxdrapi.h"

static JSBool
regexp_xdrObject(JSXDRState *xdr, JSObject **objp)
{
    JSRegExp *re;
    JSString *source;
    uint32 flagsword, modernword;
    JSObject *obj;
    jsval roots[2];
    JSTempValueRooter tvr;
    JSBool ok;

    if (xdr->mode == JSXDR_ENCODE) {
        re = (JSRegExp *) JS_GetPrivate(xdr->cx, *objp);
        if (!re)
            return JS_FALSE;
        source = re->source;
        flagsword = ((uint32)re->cloneIndex << 16) | re->flags;
        modernword = re->modern;
    }
    if (!JS_XDRString(xdr, &source) ||
        !JS_XDRUint32(xdr, &flagsword) ||
        !JS_XDRUint32(xdr, &modernword)) {
        return JS_FALSE;
    }
    if (xdr->mode == JSXDR_DECODE) {
        if (modernword > 1) {
            JS_ReportErrorNumber(xdr->cx, js_GetErrorMessage, NULL, JSMSG_BAD_SCRIPT_MAGIC);
            return JS_FALSE;
        }
        /* Compilation can invoke the embedding's branch callback. Keep both
         * the decoded source and the new instance live across that callback. */
        roots[0] = STRING_TO_JSVAL(source);
        roots[1] = JSVAL_NULL;
        JS_PUSH_TEMP_ROOT(xdr->cx, 2, roots, &tvr);
        ok = JS_FALSE;
        obj = NewRegExpInstance(xdr->cx, NULL);
        if (!obj)
            goto done;
        roots[1] = OBJECT_TO_JSVAL(obj);
        re = NewRegExpWithEdition(xdr->cx, NULL, source, (uint16)flagsword,
                                  JS_FALSE, modernword != 0);
        if (!re)
            goto done;
        if (!JS_SetPrivate(xdr->cx, obj, re)) {
            js_DestroyRegExp(xdr->cx, re);
            goto done;
        }
        /* The object owns re once installed, including on failure below. */
        if (!js_SetLastIndex(xdr->cx, obj, 0))
            goto done;
        re->cloneIndex = (uint16)(flagsword >> 16);
        *objp = obj;
        ok = JS_TRUE;
      done:
        JS_POP_TEMP_ROOT(xdr->cx, &tvr);
        return ok;
    }
    return JS_TRUE;
}

#else  /* !JS_HAS_XDR */

#define regexp_xdrObject NULL

#endif /* !JS_HAS_XDR */

static uint32
regexp_mark(JSContext *cx, JSObject *obj, void *arg)
{
    JSRegExp *re = (JSRegExp *) JS_GetPrivate(cx, obj);
    if (re)
        GC_MARK(cx, re->source, "source");
    return 0;
}

JSClass js_RegExpClass = {
    js_RegExp_str,
    JSCLASS_HAS_PRIVATE | JSCLASS_HAS_RESERVED_SLOTS(1) |
    JSCLASS_HAS_CACHED_PROTO(JSProto_RegExp),
    JS_PropertyStub,    JS_PropertyStub,
    regexp_getProperty, regexp_setProperty,
    JS_EnumerateStub,   JS_ResolveStub,
    JS_ConvertStub,     regexp_finalize,
    NULL,               NULL,
    regexp_call,        NULL,
    regexp_xdrObject,   NULL,
    regexp_mark,        0
};

static const jschar empty_regexp_ucstr[] = {'(', '?', ':', ')', 0};

JSBool
js_regexp_toString(JSContext *cx, JSObject *obj, uintN argc, jsval *argv,
                   jsval *rval)
{
    JSRegExp *re;
    const jschar *source;
    jschar *chars;
    size_t length, nflags;
    uintN flags;
    JSString *str;

    if (argv && js_IsModernGlobal(cx, js_BuiltinGlobal(cx, argv)))
        return ModernRegExpString(cx, argv, rval);
    if (!JS_InstanceOf(cx, obj, &js_RegExpClass, argv))
        return JS_FALSE;
    JS_LOCK_OBJ(cx, obj);
    re = (JSRegExp *) JS_GetPrivate(cx, obj);
    if (!re) {
        JS_UNLOCK_OBJ(cx, obj);
        *rval = STRING_TO_JSVAL(cx->runtime->emptyString);
        return JS_TRUE;
    }

    source = JSSTRING_CHARS(re->source);
    length = JSSTRING_LENGTH(re->source);
    if (length == 0) {
        source = empty_regexp_ucstr;
        length = sizeof(empty_regexp_ucstr) / sizeof(jschar) - 1;
    }
    length += 2;
    nflags = 0;
    for (flags = re->flags; flags != 0; flags &= flags - 1)
        nflags++;
    chars = (jschar*) JS_malloc(cx, (length + nflags + 1) * sizeof(jschar));
    if (!chars) {
        JS_UNLOCK_OBJ(cx, obj);
        return JS_FALSE;
    }

    chars[0] = '/';
    js_strncpy(&chars[1], source, length - 2);
    chars[length-1] = '/';
    if (nflags) {
        if (re->flags & JSREG_GLOB)
            chars[length++] = 'g';
        if (re->flags & JSREG_FOLD)
            chars[length++] = 'i';
        if (re->flags & JSREG_MULTILINE)
            chars[length++] = 'm';
        if (re->flags & JSREG_UNICODE) chars[length++] = 'u';
        if (re->flags & JSREG_STICKY)
            chars[length++] = 'y';
    }
    JS_UNLOCK_OBJ(cx, obj);
    chars[length] = 0;

    str = js_NewString(cx, chars, length, 0);
    if (!str) {
        JS_free(cx, chars);
        return JS_FALSE;
    }
    *rval = STRING_TO_JSVAL(str);
    return JS_TRUE;
}

static JSBool
regexp_compile(JSContext *cx, JSObject *obj, uintN argc, jsval *argv,
               jsval *rval)
{
    JSString *opt, *str;
    JSRegExp *oldre, *re;
    JSBool ok, ok2;
    JSObject *obj2;
    size_t length, nbytes;
    const jschar *cp, *start, *end;
    jschar *nstart, *ncp, *tmp;

    if (!JS_InstanceOf(cx, obj, &js_RegExpClass, argv))
        return JS_FALSE;
    opt = NULL;
    if (argc == 0) {
        str = cx->runtime->emptyString;
    } else {
        if (JSVAL_IS_OBJECT(argv[0])) {
            /*
             * If we get passed in a RegExp object we construct a new
             * RegExp that is a duplicate of it by re-compiling the
             * original source code. ECMA requires that it be an error
             * here if the flags are specified. (We must use the flags
             * from the original RegExp also).
             */
            obj2 = JSVAL_TO_OBJECT(argv[0]);
            if (obj2 && OBJ_GET_CLASS(cx, obj2) == &js_RegExpClass) {
                if (argc >= 2 && !JSVAL_IS_VOID(argv[1])) { /* 'flags' passed */
                    JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                                         JSMSG_NEWREGEXP_FLAGGED);
                    return JS_FALSE;
                }
                JS_LOCK_OBJ(cx, obj2);
                re = (JSRegExp *) JS_GetPrivate(cx, obj2);
                if (!re) {
                    JS_UNLOCK_OBJ(cx, obj2);
                    return JS_FALSE;
                }
                re = js_NewRegExp(cx, NULL, re->source, re->flags, JS_FALSE);
                JS_UNLOCK_OBJ(cx, obj2);
                goto created;
            }
        }
        str = JSVAL_IS_VOID(argv[0]) ? cx->runtime->emptyString
                                     : js_ValueToString(cx, argv[0]);
        if (!str)
            return JS_FALSE;
        argv[0] = STRING_TO_JSVAL(str);
        if (argc > 1) {
            if (JSVAL_IS_VOID(argv[1])) {
                opt = NULL;
            } else {
                opt = js_ValueToString(cx, argv[1]);
                if (!opt)
                    return JS_FALSE;
                argv[1] = STRING_TO_JSVAL(opt);
            }
        }

        /* Escape any naked slashes in the regexp source. */
        length = JSSTRING_LENGTH(str);
        start = JSSTRING_CHARS(str);
        end = start + length;
        nstart = ncp = NULL;
        for (cp = start; cp < end; cp++) {
            if (*cp == '/' && (cp == start || cp[-1] != '\\')) {
                nbytes = (++length + 1) * sizeof(jschar);
                if (!nstart) {
                    nstart = (jschar *) JS_malloc(cx, nbytes);
                    if (!nstart)
                        return JS_FALSE;
                    ncp = nstart + (cp - start);
                    js_strncpy(nstart, start, cp - start);
                } else {
                    tmp = (jschar *) JS_realloc(cx, nstart, nbytes);
                    if (!tmp) {
                        JS_free(cx, nstart);
                        return JS_FALSE;
                    }
                    ncp = tmp + (ncp - nstart);
                    nstart = tmp;
                }
                *ncp++ = '\\';
            }
            if (nstart)
                *ncp++ = *cp;
        }

        if (nstart) {
            /* Don't forget to store the backstop after the new string. */
            JS_ASSERT((size_t)(ncp - nstart) == length);
            *ncp = 0;
            str = js_NewString(cx, nstart, length, 0);
            if (!str) {
                JS_free(cx, nstart);
                return JS_FALSE;
            }
            argv[0] = STRING_TO_JSVAL(str);
        }
    }

    re = NewRegExpOpt(cx, NULL, str, opt, JS_FALSE,
                      JS_VERSION_IS_ES2015(cx) ||
                      (argv && js_IsModernGlobal(cx, js_BuiltinGlobal(cx, argv))));
created:
    if (!re)
        return JS_FALSE;
    JS_LOCK_OBJ(cx, obj);
    oldre = (JSRegExp *) JS_GetPrivate(cx, obj);
    ok = JS_SetPrivate(cx, obj, re);
    ok2 = js_SetLastIndex(cx, obj, 0);
    JS_UNLOCK_OBJ(cx, obj);
    if (!ok) {
        js_DestroyRegExp(cx, re);
        return JS_FALSE;
    }
    if (oldre)
        js_DestroyRegExp(cx, oldre);
    *rval = OBJECT_TO_JSVAL(obj);
    return ok2;
}

static JSBool
regexp_exec_sub(JSContext *cx, JSObject *obj, uintN argc, jsval *argv,
                JSBool test, jsval *rval)
{
    JSBool ok;
    JSRegExp *re;
    jsdouble lastIndex;
    JSString *str;
    size_t i;

    ok = JS_InstanceOf(cx, obj, &js_RegExpClass, argv);
    if (!ok)
        return JS_FALSE;
    JS_LOCK_OBJ(cx, obj);
    re = (JSRegExp *) JS_GetPrivate(cx, obj);
    if (!re) {
        JS_UNLOCK_OBJ(cx, obj);
        return JS_TRUE;
    }

    /* NB: we must reach out: after this paragraph, in order to drop re. */
    HOLD_REGEXP(cx, re);
    if (re->flags & (JSREG_GLOB | JSREG_STICKY)) {
        ok = js_GetLastIndex(cx, obj, &lastIndex);
    } else {
        lastIndex = 0;
    }
    JS_UNLOCK_OBJ(cx, obj);
    if (!ok)
        goto out;

    /* Now that obj is unlocked, it's safe to (potentially) grab the GC lock. */
    if (argc == 0 && JSVERSION_NUMBER(cx) != JSVERSION_DEFAULT &&
        !JS_VERSION_IS_ES2015(cx)) {
        str = cx->regExpStatics.pendingInput;
        if (!str) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                                 JSMSG_NO_INPUT,
                                 JS_GetStringBytes(re->source),
                                 (re->flags & JSREG_GLOB) ? "g" : "",
                                 (re->flags & JSREG_FOLD) ? "i" : "",
                                 (re->flags & JSREG_MULTILINE) ? "m" : "");
            ok = JS_FALSE;
            goto out;
        }
    } else {
        str = js_ValueToString(cx, argv[0]);
        if (!str) {
            ok = JS_FALSE;
            goto out;
        }
        argv[0] = STRING_TO_JSVAL(str);
    }

    if (lastIndex < 0 || JSSTRING_LENGTH(str) < lastIndex) {
        ok = js_SetLastIndex(cx, obj, 0);
        *rval = JSVAL_NULL;
    } else {
        i = (size_t) lastIndex;
        ok = js_ExecuteRegExp(cx, re, str, &i, test, rval);
        if (ok && (re->flags & (JSREG_GLOB | JSREG_STICKY)))
            ok = js_SetLastIndex(cx, obj, (*rval == JSVAL_NULL) ? 0 : i);
    }

out:
    DROP_REGEXP(cx, re);
    return ok;
}

static JSBool
regexp_exec(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    return regexp_exec_sub(cx, obj, argc, argv, JS_FALSE, rval);
}

static JSBool
regexp_test(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    if (!regexp_exec_sub(cx, obj, argc, argv, JS_TRUE, rval))
        return JS_FALSE;
    if (*rval != JSVAL_TRUE)
        *rval = JSVAL_FALSE;
    return JS_TRUE;
}

static JSBool SetRegExpIndexValue(JSContext *cx, JSObject *obj, jsval value);

static JSBool
RegExpBuiltinExec(JSContext *cx, JSObject *obj, JSString *str, JSObject *globalObject, jsval *rval)
{
    jsval values[3] = {STRING_TO_JSVAL(str), JSVAL_VOID, JSVAL_VOID};
    JSTempValueRooter root;
    jsdouble lastIndex;
    JSBool global, sticky, ok = JS_FALSE;
    JSRegExp *re = NULL;
    size_t index;
    if (OBJ_GET_CLASS(cx, obj) != &js_RegExpClass || !JS_GetPrivate(cx, obj))
        return RegExpReceiverError(cx, "exec");
    JS_PUSH_TEMP_ROOT(cx, 3, values, &root);
    if (!JS_GetProperty(cx, obj, "lastIndex", &values[1]) ||
        !js_ValueToNumber(cx, values[1], &lastIndex)) goto out;
    lastIndex = js_DoubleToInteger(lastIndex);
    if (lastIndex < 0) lastIndex = 0;
    else if (lastIndex > 9007199254740991.0) lastIndex = 9007199254740991.0;
    if (!JS_GetProperty(cx, obj, "global", &values[1]) ||
        !js_ValueToBoolean(cx, values[1], &global) ||
        !JS_GetProperty(cx, obj, "sticky", &values[1]) ||
        !js_ValueToBoolean(cx, values[1], &sticky)) goto out;
    if (!global && !sticky) lastIndex = 0;
    if (lastIndex > JSSTRING_LENGTH(str)) {
        *rval = JSVAL_NULL;
        ok = SetRegExpIndexValue(cx, obj, JSVAL_ZERO); goto out;
    }
    /* Snapshot the matcher only after user-visible conversions and gets. */
    JS_LOCK_OBJ(cx, obj);
    re = (JSRegExp *)JS_GetPrivate(cx, obj);
    if (re) {
        HOLD_REGEXP(cx, re);
        values[2] = STRING_TO_JSVAL(re->source);
    }
    JS_UNLOCK_OBJ(cx, obj);
    if (!re) { RegExpReceiverError(cx, "exec"); goto out; }
    index = (size_t)lastIndex;
    if (!ExecuteRegExp(cx, re, str, &index, JS_FALSE, sticky, globalObject, rval)) goto out;
    if (*rval == JSVAL_NULL) ok = SetRegExpIndexValue(cx, obj, JSVAL_ZERO);
    else if (global || sticky)
        ok = js_NewNumberValue(cx, index, &values[1]) &&
             SetRegExpIndexValue(cx, obj, values[1]);
    else ok = JS_TRUE;
  out:
    if (re) DROP_REGEXP(cx, re);
    JS_POP_TEMP_ROOT(cx, &root);
    return ok;
}

static JSBool
RegExpModernExec(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    JSString *str;
    if (JSVAL_IS_PRIMITIVE(argv[-1])) return RegExpReceiverError(cx, "exec");
    obj = JSVAL_TO_OBJECT(argv[-1]);
    if (OBJ_GET_CLASS(cx, obj) != &js_RegExpClass || !JS_GetPrivate(cx, obj))
        return RegExpReceiverError(cx, "exec");
    str = js_ValueToString(cx, argv[0]);
    if (!str) return JS_FALSE;
    argv[0] = STRING_TO_JSVAL(str);
    return RegExpBuiltinExec(cx, obj, str, js_BuiltinGlobal(cx, argv), rval);
}

/* ES2015 RegExpExec preserves overridden exec methods and their receivers. */
static JSBool
RegExpExecMethod(JSContext *cx, JSObject *obj, JSString *str, jsval callee,
                  jsval *rval)
{
    jsval values[4] = {callee, OBJECT_TO_JSVAL(obj), STRING_TO_JSVAL(str), JSVAL_VOID};
    JSTempValueRooter root;
    JSBool ok = JS_FALSE;
    JS_PUSH_TEMP_ROOT(cx, 4, values, &root);
    if (!JS_GetProperty(cx, obj, "exec", &values[3])) goto out;
    if (js_IsCallable(cx, values[3])) {
        if (!js_InternalCall(cx, obj, values[3], 1, &values[2], rval)) goto out;
        if (!JSVAL_IS_OBJECT(*rval)) {
            RegExpReceiverError(cx, "exec result"); goto out;
        }
        ok = JS_TRUE;
    } else {
        ok = RegExpBuiltinExec(cx, obj, str, js_BuiltinGlobal(cx, &values[2]), rval);
    }
  out:
    JS_POP_TEMP_ROOT(cx, &root);
    return ok;
}

static JSBool
SetRegExpIndexValue(JSContext *cx, JSObject *obj, jsval value)
{
    JSAtom *atom;
    JSTempValueRooter root;
    JSBool ok;
    JS_PUSH_TEMP_ROOT(cx, 1, &value, &root);
    atom = js_Atomize(cx, "lastIndex", 9, 0);
    ok = atom && js_SetPropertyOrThrow(cx, obj, ATOM_TO_JSID(atom), &value);
    JS_POP_TEMP_ROOT(cx, &root);
    return ok;
}

static JSBool
RegExpModernTest(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    JSString *str;
    JSTempValueRooter root;
    JSBool ok;
    if (JSVAL_IS_PRIMITIVE(argv[-1])) return RegExpReceiverError(cx, "test");
    obj = JSVAL_TO_OBJECT(argv[-1]);
    str = js_ValueToString(cx, argv[0]);
    if (!str) return JS_FALSE;
    JS_PUSH_TEMP_ROOT_STRING(cx, str, &root);
    ok = RegExpExecMethod(cx, obj, str, argv[-2], rval);
    if (ok) *rval = BOOLEAN_TO_JSVAL(*rval != JSVAL_NULL);
    JS_POP_TEMP_ROOT(cx, &root);
    return ok;
}

static JSBool
RegExpSymbolSearch(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    jsval values[3] = {JSVAL_VOID, JSVAL_VOID, JSVAL_VOID};
    JSTempValueRooter root;
    JSString *str;
    JSBool ok = JS_FALSE;
    if (JSVAL_IS_PRIMITIVE(argv[-1])) return RegExpReceiverError(cx, "[Symbol.search]");
    obj = JSVAL_TO_OBJECT(argv[-1]);
    JS_PUSH_TEMP_ROOT(cx, 3, values, &root);
    str = js_ValueToString(cx, argv[0]);
    if (!str) goto out;
    values[0] = STRING_TO_JSVAL(str);
    if (!JS_GetProperty(cx, obj, "lastIndex", &values[1]) ||
        !SetRegExpIndexValue(cx, obj, JSVAL_ZERO) ||
        !RegExpExecMethod(cx, obj, str, argv[-2], &values[2])) goto out;
    /* ES2015 always performs both writes, and does not restore on exec throw. */
    if (!SetRegExpIndexValue(cx, obj, values[1])) goto out;
    if (values[2] == JSVAL_NULL) {
        *rval = INT_TO_JSVAL(-1); ok = JS_TRUE;
    } else ok = JS_GetProperty(cx, JSVAL_TO_OBJECT(values[2]), "index", rval);
  out:
    JS_POP_TEMP_ROOT(cx, &root);
    return ok;
}

static JSBool
RegExpSymbolMatch(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    jsval values[5] = {JSVAL_VOID, JSVAL_VOID, JSVAL_VOID, JSVAL_VOID, JSVAL_VOID};
    JSTempValueRooter root;
    JSString *str, *matched;
    JSObject *global, *proto, *array;
    JSBool isGlobal, unicode, ok = JS_FALSE;
    jsdouble index;
    jsuint n = 0;
    jsid id;
    const jschar *chars;
    if (JSVAL_IS_PRIMITIVE(argv[-1])) return RegExpReceiverError(cx, "[Symbol.match]");
    obj = JSVAL_TO_OBJECT(argv[-1]);
    JS_PUSH_TEMP_ROOT(cx, 5, values, &root);
    str = js_ValueToString(cx, argv[0]);
    if (!str) goto out;
    values[0] = STRING_TO_JSVAL(str);
    if (!JS_GetProperty(cx, obj, "global", &values[1]) ||
        !js_ValueToBoolean(cx, values[1], &isGlobal)) goto out;
    if (!isGlobal) {
        ok = RegExpExecMethod(cx, obj, str, argv[-2], rval); goto out;
    }
    if (!JS_GetProperty(cx, obj, "unicode", &values[1]) ||
        !js_ValueToBoolean(cx, values[1], &unicode) ||
        !SetRegExpIndexValue(cx, obj, JSVAL_ZERO)) goto out;
    global = js_BuiltinGlobal(cx, argv);
    proto = js_BuiltinPrototype(cx, global, JSProto_Array);
    if (!proto) goto out;
    array = js_NewArrayObjectWithProto(cx, 0, NULL, proto, global);
    if (!array) goto out;
    values[4] = OBJECT_TO_JSVAL(array);
    for (;;) {
        if (cx->branchCallback && (n & 127) == 0 && !cx->branchCallback(cx, NULL)) goto out;
        if (!RegExpExecMethod(cx, obj, str, argv[-2], &values[2])) goto out;
        if (values[2] == JSVAL_NULL) {
            *rval = n ? values[4] : JSVAL_NULL; ok = JS_TRUE; goto out;
        }
        if (!JS_GetElement(cx, JSVAL_TO_OBJECT(values[2]), 0, &values[3])) goto out;
        matched = js_ValueToString(cx, values[3]);
        if (!matched) goto out;
        values[3] = STRING_TO_JSVAL(matched);
        if (n == (jsuint)-1) { JS_ReportOutOfMemory(cx); goto out; }
        if (!js_ArrayLikeIndex(cx, n, &id) ||
            !OBJ_DEFINE_PROPERTY(cx, array, id, values[3], JS_PropertyStub,
                                  JS_PropertyStub, JSPROP_ENUMERATE, NULL)) goto out;
        ++n;
        if (!JSSTRING_LENGTH(matched)) {
            if (!JS_GetProperty(cx, obj, "lastIndex", &values[1]) ||
                !js_ValueToNumber(cx, values[1], &index)) goto out;
            index = js_DoubleToInteger(index);
            if (index < 0) index = 0;
            else if (index > 9007199254740991.0) index = 9007199254740991.0;
            chars = JSSTRING_CHARS(str);
            if (unicode && index + 1 < JSSTRING_LENGTH(str) &&
                chars[(size_t)index] >= 0xd800 && chars[(size_t)index] <= 0xdbff &&
                chars[(size_t)index + 1] >= 0xdc00 && chars[(size_t)index + 1] <= 0xdfff)
                index += 2;
            else index += 1;
            if (!js_NewNumberValue(cx, index, &values[1]) ||
                !SetRegExpIndexValue(cx, obj, values[1])) goto out;
        }
    }
  out:
    JS_POP_TEMP_ROOT(cx, &root);
    return ok;
}

/* RegExp index and capture counts use ToLength. */
static JSBool
RegExpToLength(JSContext *cx, jsval value, jsdouble *length)
{
    if (!js_ValueToNumber(cx, value, length)) return JS_FALSE;
    *length = js_DoubleToInteger(*length);
    if (*length < 0) *length = 0;
    else if (*length > 9007199254740991.0) *length = 9007199254740991.0;
    return JS_TRUE;
}

static jsdouble
RegExpAdvanceIndex(JSString *str, jsdouble index, JSBool unicode)
{
    const jschar *chars = JSSTRING_CHARS(str);
    if (unicode && index + 1 < JSSTRING_LENGTH(str) &&
        chars[(size_t)index] >= 0xd800 && chars[(size_t)index] <= 0xdbff &&
        chars[(size_t)index + 1] >= 0xdc00 && chars[(size_t)index + 1] <= 0xdfff)
        return index + 2;
    return index + 1;
}

static JSBool
RegExpAppendElement(JSContext *cx, JSObject *array, jsdouble index, jsval value)
{
    jsid id;
    if (index >= 4294967295.0) { JS_ReportOutOfMemory(cx); return JS_FALSE; }
    return js_ArrayLikeIndex(cx, index, &id) &&
           OBJ_DEFINE_PROPERTY(cx, array, id, value, JS_PropertyStub,
                               JS_PropertyStub, JSPROP_ENUMERATE, NULL);
}

static JSBool
RegExpSymbolSplit(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    jsval values[7] = {JSVAL_VOID, JSVAL_VOID, JSVAL_VOID, JSVAL_VOID,
                      JSVAL_VOID, JSVAL_VOID, JSVAL_VOID};
    JSTempValueRooter root;
    JSObject *global, *ctor, *proto, *splitter, *array;
    JSString *str, *flags, *suffix, *sub;
    JSBool unicode = JS_FALSE, sticky = JS_FALSE, ok = JS_FALSE;
    jsdouble limit = 9007199254740991.0, p = 0, q = 0, e, count = 0, captures, i;
    size_t size, j;
    uint32 iterations = 0, limit32 = (uint32)-1;
    jsid id;
    jsval *base, *oldsp;
    JSStackFrame *frame;
    void *mark;
    if (JSVAL_IS_PRIMITIVE(argv[-1])) return RegExpReceiverError(cx, "[Symbol.split]");
    obj = JSVAL_TO_OBJECT(argv[-1]);
    JS_PUSH_TEMP_ROOT(cx, 7, values, &root);
    str = js_ValueToString(cx, argv[0]);
    if (!str) goto out;
    values[0] = STRING_TO_JSVAL(str);
    global = js_BuiltinGlobal(cx, argv);
    if (!JS_GetProperty(cx, obj, "constructor", &values[1])) goto out;
    if (!JSVAL_IS_VOID(values[1])) {
        if (JSVAL_IS_PRIMITIVE(values[1])) { RegExpReceiverError(cx, "constructor"); goto out; }
        if (!js_WellKnownSymbolId(cx, JS_WKS_SPECIES, &id) ||
            !OBJ_GET_PROPERTY(cx, JSVAL_TO_OBJECT(values[1]), id, &values[1])) goto out;
    }
    if (JSVAL_IS_VOID(values[1]) || JSVAL_IS_NULL(values[1])) {
        ctor = js_GetCachedClassObject(cx, global, JSProto_RegExp);
        if (!ctor) {
            if (!js_BuiltinPrototype(cx, global, JSProto_RegExp)) goto out;
            ctor = js_GetCachedClassObject(cx, global, JSProto_RegExp);
        }
        if (!ctor) goto out;
        values[1] = OBJECT_TO_JSVAL(ctor);
    } else if (!js_IsConstructor(cx, values[1])) {
        RegExpReceiverError(cx, "species constructor"); goto out;
    }
    if (!JS_GetProperty(cx, obj, "flags", &values[2])) goto out;
    flags = js_ValueToString(cx, values[2]);
    if (!flags) goto out;
    values[2] = STRING_TO_JSVAL(flags);
    for (j = 0; j < JSSTRING_LENGTH(flags); ++j) {
        if (JSSTRING_CHARS(flags)[j] == 'u') unicode = JS_TRUE;
        if (JSSTRING_CHARS(flags)[j] == 'y') sticky = JS_TRUE;
    }
    if (!sticky) {
        suffix = JS_NewStringCopyZ(cx, "y");
        if (!suffix) goto out;
        values[3] = STRING_TO_JSVAL(suffix);
        flags = js_ConcatStrings(cx, flags, suffix);
        if (!flags) goto out;
        values[2] = STRING_TO_JSVAL(flags);
    }
    base = js_AllocStack(cx, 4, &mark);
    if (!base) goto out;
    base[0] = values[1]; base[1] = JSVAL_NULL;
    base[2] = argv[-1]; base[3] = values[2];
    frame = cx->fp; oldsp = frame->sp; frame->sp = base + 4;
    ok = js_InvokeConstructorWithNewTarget(cx, base, 2, JSVAL_TO_OBJECT(values[1]));
    if (ok) values[3] = base[0];
    frame->sp = oldsp; js_FreeStack(cx, mark);
    if (!ok) goto out;
    ok = JS_FALSE;
    splitter = JSVAL_TO_OBJECT(values[3]);
    proto = js_BuiltinPrototype(cx, global, JSProto_Array);
    if (!proto) goto out;
    array = js_NewArrayObjectWithProto(cx, 0, NULL, proto, global);
    if (!array) goto out;
    values[4] = OBJECT_TO_JSVAL(array);
    /* The pinned ES2015 corpus includes the corrected ToUint32 limit. */
    if (!JSVAL_IS_VOID(argv[1])) {
        if (!js_ValueToNumber(cx, argv[1], &limit) ||
            !js_DoubleToECMAUint32(cx, limit, &limit32)) goto out;
    }
    limit = limit32;
    size = JSSTRING_LENGTH(str);
    if (!limit) goto done;
    if (!size) {
        if (!RegExpExecMethod(cx, splitter, str, argv[-2], &values[5])) goto out;
        if (values[5] == JSVAL_NULL && !RegExpAppendElement(cx, array, 0, values[0])) goto out;
        goto done;
    }
    while (q < size) {
        if (cx->branchCallback && !(iterations++ & 127) && !cx->branchCallback(cx, NULL)) goto out;
        if (!js_NewNumberValue(cx, q, &values[6]) ||
            !SetRegExpIndexValue(cx, splitter, values[6]) ||
            !RegExpExecMethod(cx, splitter, str, argv[-2], &values[5])) goto out;
        if (values[5] == JSVAL_NULL) { q = RegExpAdvanceIndex(str, q, unicode); continue; }
        if (!JS_GetProperty(cx, splitter, "lastIndex", &values[6]) ||
            !RegExpToLength(cx, values[6], &e)) goto out;
        if (e == p) { q = RegExpAdvanceIndex(str, q, unicode); continue; }
        sub = js_NewDependentString(cx, str, (size_t)p, (size_t)(q - p), 0);
        if (!sub) goto out;
        values[6] = STRING_TO_JSVAL(sub);
        if (!RegExpAppendElement(cx, array, count++, values[6])) goto out;
        if (count == limit) goto done;
        p = e;
        if (!js_ArrayLikeLength(cx, JSVAL_TO_OBJECT(values[5]), &captures)) goto out;
        for (i = 1; i < captures; ++i) {
            if (cx->branchCallback && !(iterations++ & 127) && !cx->branchCallback(cx, NULL)) goto out;
            if (!js_ArrayLikeIndex(cx, i, &id) ||
                !OBJ_GET_PROPERTY(cx, JSVAL_TO_OBJECT(values[5]), id, &values[6]) ||
                !RegExpAppendElement(cx, array, count++, values[6])) goto out;
            if (count == limit) goto done;
        }
        q = p;
    }
    /* A custom exec may set an index beyond the string. Never form an
     * out-of-bounds dependent string for the final empty suffix. */
    if (p > size) p = size;
    sub = js_NewDependentString(cx, str, (size_t)p, size - (size_t)p, 0);
    if (!sub) goto out;
    values[6] = STRING_TO_JSVAL(sub);
    if (!RegExpAppendElement(cx, array, count, values[6])) goto out;
  done:
    *rval = values[4]; ok = JS_TRUE;
  out:
    JS_POP_TEMP_ROOT(cx, &root);
    return ok;
}

/* A checked native buffer avoids quadratic concatenation of replacement parts. */
typedef struct RegExpText {
    jschar *chars;
    size_t length, capacity;
} RegExpText;

static JSBool
AppendRegExpText(JSContext *cx, RegExpText *text, const jschar *chars, size_t length)
{
    size_t needed, capacity;
    jschar *newChars;
    if (length > JSSTRING_LENGTH_MASK - text->length ||
        text->length + length >= ((size_t)-1) / sizeof(jschar)) {
        JS_ReportOutOfMemory(cx); return JS_FALSE;
    }
    needed = text->length + length + 1;
    if (needed > text->capacity) {
        capacity = text->capacity ? text->capacity : 64;
        while (capacity < needed) {
            if (capacity > ((size_t)-1) / sizeof(jschar) / 2) { capacity = needed; break; }
            capacity *= 2;
        }
        newChars = (jschar *)JS_realloc(cx, text->chars, capacity * sizeof(jschar));
        if (!newChars) return JS_FALSE;
        text->chars = newChars; text->capacity = capacity;
    }
    if (length) memcpy(text->chars + text->length, chars, length * sizeof(jschar));
    text->length += length; text->chars[text->length] = 0;
    return JS_TRUE;
}

/* captures contains already-converted strings or undefined, starting at 0. */
static JSBool
RegExpSubstitution(JSContext *cx, RegExpText *text, JSString *matched, JSString *str,
                    size_t position, JSObject *captures, jsdouble count, JSString *replacement)
{
    size_t i, length = JSSTRING_LENGTH(replacement), end, n, skip;
    const jschar *chars = JSSTRING_CHARS(replacement), *piece;
    JSString *capture;
    jsval value = JSVAL_VOID;
    JSTempValueRooter root;
    JSBool ok = JS_FALSE;
    uintN number, second;
    JS_PUSH_TEMP_ROOT(cx, 1, &value, &root);
    for (i = 0; i < length; i += skip) {
        piece = chars + i; n = 1; skip = 1;
        if (chars[i] == '$' && i + 1 < length) {
            switch (chars[i + 1]) {
              case '$': skip = 2; break;
              case '&': piece = JSSTRING_CHARS(matched); n = JSSTRING_LENGTH(matched); skip = 2; break;
              case '`': piece = JSSTRING_CHARS(str); n = position; skip = 2; break;
              case '\'':
                end = position + JSSTRING_LENGTH(matched);
                if (end > JSSTRING_LENGTH(str)) end = JSSTRING_LENGTH(str);
                piece = JSSTRING_CHARS(str) + end; n = JSSTRING_LENGTH(str) - end; skip = 2; break;
              default:
                if (chars[i + 1] >= '0' && chars[i + 1] <= '9') {
                    number = chars[i + 1] - '0';
                    if (i + 2 < length && chars[i + 2] >= '0' && chars[i + 2] <= '9') {
                        second = number * 10 + chars[i + 2] - '0';
                        if (second && second <= count) { number = second; skip = 3; }
                    }
                    if (number && number <= count) {
                        if (skip == 1) skip = 2;
                        if (!JS_GetElement(cx, captures, number - 1, &value)) goto out;
                        if (JSVAL_IS_VOID(value)) n = 0;
                        else {
                            capture = JSVAL_TO_STRING(value);
                            piece = JSSTRING_CHARS(capture); n = JSSTRING_LENGTH(capture);
                        }
                    }
                }
                break;
            }
        }
        if (!AppendRegExpText(cx, text, piece, n)) goto out;
    }
    ok = JS_TRUE;
  out:
    JS_POP_TEMP_ROOT(cx, &root);
    return ok;
}

static JSBool
RegExpSymbolReplace(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    jsval values[9] = {JSVAL_VOID, JSVAL_VOID, JSVAL_VOID, JSVAL_VOID, JSVAL_VOID,
                      JSVAL_VOID, JSVAL_VOID, JSVAL_VOID, JSVAL_VOID};
    JSTempValueRooter root;
    JSString *str, *replacement = NULL, *matched, *converted, *output;
    JSObject *global, *proto, *results, *captures, *result;
    JSBool functional, isGlobal, unicode = JS_FALSE, ok = JS_FALSE;
    jsdouble index, length, count = 0, r, c;
    size_t position, next = 0, size, matchLength;
    uint32 iterations = 0;
    jsid id;
    jsval *args;
    void *mark;
    RegExpText text = {NULL, 0, 0}, substitution = {NULL, 0, 0};
    if (JSVAL_IS_PRIMITIVE(argv[-1])) return RegExpReceiverError(cx, "[Symbol.replace]");
    obj = JSVAL_TO_OBJECT(argv[-1]);
    JS_PUSH_TEMP_ROOT(cx, 9, values, &root);
    str = js_ValueToString(cx, argv[0]);
    if (!str) goto out;
    values[0] = STRING_TO_JSVAL(str); size = JSSTRING_LENGTH(str);
    functional = js_IsCallable(cx, argv[1]);
    values[1] = argv[1];
    if (!functional) {
        replacement = js_ValueToString(cx, values[1]);
        if (!replacement) goto out;
        values[1] = STRING_TO_JSVAL(replacement);
    }
    if (!JS_GetProperty(cx, obj, "global", &values[2]) ||
        !js_ValueToBoolean(cx, values[2], &isGlobal)) goto out;
    if (isGlobal && (!JS_GetProperty(cx, obj, "unicode", &values[2]) ||
        !js_ValueToBoolean(cx, values[2], &unicode) ||
        !SetRegExpIndexValue(cx, obj, JSVAL_ZERO))) goto out;
    global = js_BuiltinGlobal(cx, argv);
    proto = js_BuiltinPrototype(cx, global, JSProto_Array);
    if (!proto) goto out;
    results = js_NewArrayObjectWithProto(cx, 0, NULL, proto, global);
    if (!results) goto out;
    values[3] = OBJECT_TO_JSVAL(results);
    for (;;) {
        if (cx->branchCallback && !(iterations++ & 127) && !cx->branchCallback(cx, NULL)) goto out;
        if (!RegExpExecMethod(cx, obj, str, argv[-2], &values[2])) goto out;
        if (values[2] == JSVAL_NULL) break;
        if (!RegExpAppendElement(cx, results, count++, values[2])) goto out;
        if (!isGlobal) break;
        if (!JS_GetElement(cx, JSVAL_TO_OBJECT(values[2]), 0, &values[4])) goto out;
        matched = js_ValueToString(cx, values[4]);
        if (!matched) goto out;
        values[4] = STRING_TO_JSVAL(matched);
        if (!JSSTRING_LENGTH(matched)) {
            if (!JS_GetProperty(cx, obj, "lastIndex", &values[5]) ||
                !RegExpToLength(cx, values[5], &index) ||
                !js_NewNumberValue(cx, RegExpAdvanceIndex(str, index, unicode), &values[5]) ||
                !SetRegExpIndexValue(cx, obj, values[5])) goto out;
        }
    }
    for (r = 0; r < count; ++r) {
        if (cx->branchCallback && !(iterations++ & 127) && !cx->branchCallback(cx, NULL)) goto out;
        if (!js_ArrayLikeIndex(cx, r, &id) || !OBJ_GET_PROPERTY(cx, results, id, &values[2])) goto out;
        result = JSVAL_TO_OBJECT(values[2]);
        if (!js_ArrayLikeLength(cx, result, &length)) goto out;
        if (length) --length;
        if (!JS_GetElement(cx, result, 0, &values[4])) goto out;
        matched = js_ValueToString(cx, values[4]);
        if (!matched) goto out;
        values[4] = STRING_TO_JSVAL(matched); matchLength = JSSTRING_LENGTH(matched);
        if (!JS_GetProperty(cx, result, "index", &values[5]) ||
            !js_ValueToNumber(cx, values[5], &index)) goto out;
        index = js_DoubleToInteger(index);
        if (index < 0) index = 0;
        else if (index > size) index = (jsdouble)size;
        position = (size_t)index;
        captures = js_NewArrayObjectWithProto(cx, 0, NULL, proto, global);
        if (!captures) goto out;
        values[6] = OBJECT_TO_JSVAL(captures);
        for (c = 0; c < length; ++c) {
            if (cx->branchCallback && !(iterations++ & 127) && !cx->branchCallback(cx, NULL)) goto out;
            if (!js_ArrayLikeIndex(cx, c + 1, &id) || !OBJ_GET_PROPERTY(cx, result, id, &values[7])) goto out;
            if (!JSVAL_IS_VOID(values[7])) {
                converted = js_ValueToString(cx, values[7]);
                if (!converted) goto out;
                values[7] = STRING_TO_JSVAL(converted);
            }
            if (!RegExpAppendElement(cx, captures, c, values[7])) goto out;
        }
        substitution.length = 0;
        if (functional) {
            if (length >= ARRAY_INIT_LIMIT - 3) {
                JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_TOO_MANY_FUN_ARGS); goto out;
            }
            args = js_AllocStack(cx, (uintN)length + 3, &mark);
            if (!args) goto out;
            for (c = 0; c < length + 3; ++c) args[(uintN)c] = JSVAL_VOID;
            args[0] = values[4];
            for (c = 0; c < length; ++c) {
                if (!JS_GetElement(cx, captures, (jsuint)c, &args[(uintN)c + 1])) break;
            }
            if (c == length && js_NewNumberValue(cx, position, &args[(uintN)length + 1])) {
                args[(uintN)length + 2] = values[0];
                ok = js_InternalInvokeValue(cx, JSVAL_VOID, values[1], 0, (uintN)length + 3, args, &values[8]);
            }
            js_FreeStack(cx, mark);
            if (!ok) goto out;
            ok = JS_FALSE;
            converted = js_ValueToString(cx, values[8]);
            if (!converted) goto out;
            values[8] = STRING_TO_JSVAL(converted);
            if (!AppendRegExpText(cx, &substitution, JSSTRING_CHARS(converted), JSSTRING_LENGTH(converted))) goto out;
        } else if (!RegExpSubstitution(cx, &substitution, matched, str, position, captures, length, replacement)) goto out;
        if (position >= next) {
            if (!AppendRegExpText(cx, &text, JSSTRING_CHARS(str) + next, position - next) ||
                !AppendRegExpText(cx, &text, substitution.chars, substitution.length)) goto out;
            next = position + matchLength;
        }
    }
    if (next < size && !AppendRegExpText(cx, &text, JSSTRING_CHARS(str) + next, size - next)) goto out;
    output = JS_NewUCStringCopyN(cx, text.chars ? text.chars : JSSTRING_CHARS(cx->runtime->emptyString), text.length);
    if (!output) goto out;
    *rval = STRING_TO_JSVAL(output); ok = JS_TRUE;
  out:
    JS_free(cx, text.chars); JS_free(cx, substitution.chars);
    JS_POP_TEMP_ROOT(cx, &root);
    return ok;
}

JSBool
js_StringReplaceES2015(JSContext *cx, jsval *argv, jsval *rval)
{
    jsval values[5] = {JSVAL_VOID, JSVAL_VOID, JSVAL_VOID, JSVAL_VOID, JSVAL_VOID};
    jsval args[3] = {argv[-1], argv[1], JSVAL_VOID};
    JSTempValueRooter root;
    JSObject *global = js_BuiltinGlobal(cx, argv), *obj;
    JSProtoKey key;
    JSString *str, *search, *replacement = NULL, *output;
    jsid id;
    size_t size, searchLength, position;
    JSBool functional, ok = JS_FALSE;
    RegExpText text = {NULL, 0, 0};
    if (JSVAL_IS_NULL(argv[-1]) || JSVAL_IS_VOID(argv[-1])) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_OBJECT_REQUIRED);
        return JS_FALSE;
    }
    JS_PUSH_TEMP_ROOT(cx, 5, values, &root);
    if (!js_WellKnownSymbolId(cx, JS_WKS_REPLACE, &id)) goto out;
    if (!JSVAL_IS_NULL(argv[0]) && !JSVAL_IS_VOID(argv[0])) {
        if (JSVAL_IS_PRIMITIVE(argv[0])) {
            key = JSVAL_IS_SYMBOL(argv[0]) ? JSProto_Symbol :
                  JSVAL_IS_STRING(argv[0]) ? JSProto_String :
                  JSVAL_IS_BOOLEAN(argv[0]) ? JSProto_Boolean : JSProto_Number;
            obj = js_BuiltinPrototype(cx, global, key);
            if (!obj || !js_GetPropertyValue(cx, obj, argv[0], id, &values[3])) goto out;
        } else if (!OBJ_GET_PROPERTY(cx, JSVAL_TO_OBJECT(argv[0]), id, &values[3])) goto out;
        if (!JSVAL_IS_VOID(values[3]) && !JSVAL_IS_NULL(values[3])) {
            if (!js_IsCallable(cx, values[3])) {
                JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_NOT_FUNCTION, "replace symbol method");
                goto out;
            }
            ok = js_InternalInvokeValue(cx, argv[0], values[3], 0, 2, args, rval);
            goto out;
        }
    }
    str = js_ValueToString(cx, argv[-1]);
    if (!str) goto out;
    values[0] = STRING_TO_JSVAL(str);
    search = js_ValueToString(cx, argv[0]);
    if (!search) goto out;
    values[1] = STRING_TO_JSVAL(search);
    functional = js_IsCallable(cx, argv[1]); values[2] = argv[1];
    if (!functional) {
        replacement = js_ValueToString(cx, argv[1]);
        if (!replacement) goto out;
        values[2] = STRING_TO_JSVAL(replacement);
    }
    size = JSSTRING_LENGTH(str); searchLength = JSSTRING_LENGTH(search);
    if (searchLength > size) { *rval = values[0]; ok = JS_TRUE; goto out; }
    for (position = 0; position <= size - searchLength; ++position) {
        if (cx->branchCallback && !(position & 127) && !cx->branchCallback(cx, NULL)) goto out;
        if (!memcmp(JSSTRING_CHARS(str) + position, JSSTRING_CHARS(search), searchLength * sizeof(jschar))) break;
    }
    if (position > size - searchLength) { *rval = values[0]; ok = JS_TRUE; goto out; }
    if (!AppendRegExpText(cx, &text, JSSTRING_CHARS(str), position)) goto out;
    if (functional) {
        args[0] = values[1]; args[2] = values[0];
        if (!js_NewNumberValue(cx, position, &values[4])) goto out;
        args[1] = values[4];
        if (!js_InternalInvokeValue(cx, JSVAL_VOID, values[2], 0, 3, args, &values[3])) goto out;
        replacement = js_ValueToString(cx, values[3]);
        if (!replacement) goto out;
        values[3] = STRING_TO_JSVAL(replacement);
        if (!AppendRegExpText(cx, &text, JSSTRING_CHARS(replacement), JSSTRING_LENGTH(replacement))) goto out;
    } else if (!RegExpSubstitution(cx, &text, search, str, position, NULL, 0, replacement)) goto out;
    if (!AppendRegExpText(cx, &text, JSSTRING_CHARS(str) + position + searchLength,
                          size - position - searchLength)) goto out;
    output = JS_NewUCStringCopyN(cx, text.chars, text.length);
    if (!output) goto out;
    *rval = STRING_TO_JSVAL(output); ok = JS_TRUE;
  out:
    JS_free(cx, text.chars); JS_POP_TEMP_ROOT(cx, &root);
    return ok;
}

static JSBool
InitRegExpProtocols(JSContext *cx, JSObject *global, JSObject *proto)
{
    static struct { JSWellKnownSymbol symbol; const char *name; JSNative native; } methods[] = {
        {JS_WKS_MATCH, "[Symbol.match]", RegExpSymbolMatch},
        {JS_WKS_SEARCH, "[Symbol.search]", RegExpSymbolSearch},
        {JS_WKS_SPLIT, "[Symbol.split]", RegExpSymbolSplit},
        {JS_WKS_REPLACE, "[Symbol.replace]", RegExpSymbolReplace}
    };
    JSFunction *fun;
    jsval value;
    jsid id;
    uintN i;
    JSTempValueRooter root;
    JSBool ok;
    for (i = 0; i < sizeof(methods) / sizeof(methods[0]); ++i) {
        if (!js_WellKnownSymbolId(cx, methods[i].symbol, &id)) return JS_FALSE;
        fun = JS_NewFunction(cx, methods[i].native, (methods[i].symbol == JS_WKS_SPLIT || methods[i].symbol == JS_WKS_REPLACE) ? 2 : 1, JSFUN_STRICT | JSFUN_NO_CONSTRUCT,
                             global, methods[i].name);
        if (!fun) return JS_FALSE;
        JS_PUSH_TEMP_ROOT_OBJECT(cx, fun->object, &root);
        ok = OBJ_DEFINE_PROPERTY(cx, proto, id, OBJECT_TO_JSVAL(fun->object),
                                  JS_PropertyStub, JS_PropertyStub, 0, NULL);
        JS_POP_TEMP_ROOT(cx, &root);
        if (!ok) return JS_FALSE;
    }
    if (!JS_GetProperty(cx, proto, "test", &value)) return JS_FALSE;
    fun = (JSFunction *)JS_GetPrivate(cx, JSVAL_TO_OBJECT(value));
    fun->u.n.native = RegExpModernTest;
    fun->flags |= JSFUN_STRICT;
    if (!JS_GetProperty(cx, proto, "exec", &value)) return JS_FALSE;
    fun = (JSFunction *)JS_GetPrivate(cx, JSVAL_TO_OBJECT(value));
    fun->u.n.native = RegExpModernExec;
    fun->flags |= JSFUN_STRICT;
    return JS_TRUE;
}

static JSFunctionSpec regexp_methods[] = {
#if JS_HAS_TOSOURCE
    {js_toSource_str,   js_regexp_toString,     0,0,0},
#endif
    {js_toString_str,   js_regexp_toString,     0,0,0},
    {"compile",         regexp_compile,         1,0,0},
    {"exec",            regexp_exec,            1,0,0},
    {"test",            regexp_test,            1,0,0},
    {0,0,0,0,0}
};

static JSBool
RegExp(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    if (!(cx->fp->flags & JSFRAME_CONSTRUCTING)) {
        /*
         * If first arg is regexp and no flags are given, just return the arg.
         * (regexp_compile detects the regexp + flags case and throws a
         * TypeError.)  See 10.15.3.1.
         */
        if ((argc < 2 || JSVAL_IS_VOID(argv[1])) &&
            !JSVAL_IS_PRIMITIVE(argv[0]) &&
            OBJ_GET_CLASS(cx, JSVAL_TO_OBJECT(argv[0])) == &js_RegExpClass) {
            *rval = argv[0];
            return JS_TRUE;
        }

        /* Otherwise, replace obj with a new RegExp object. */
        obj = NewRegExpInstance(cx, NULL);
        if (!obj)
            return JS_FALSE;

        /*
         * regexp_compile does not use rval to root its temporaries
         * so we can use it to root obj.
         */
        *rval = OBJECT_TO_JSVAL(obj);
    }
    if (!regexp_compile(cx, obj, argc, argv, rval)) return JS_FALSE;
    if ((cx->fp->flags & JSFRAME_NEW_TARGET) && cx->fp->newTarget != cx->fp->callee)
        return JS_DefinePropertyWithTinyId(cx, obj, "lastIndex", REGEXP_LAST_INDEX,
                    JSVAL_ZERO, regexp_getProperty, regexp_setProperty, JSPROP_PERMANENT);
    return JS_TRUE;
}

static JSString *
RegExpFlagsString(JSContext *cx, uintN flags)
{
    jschar chars[5];
    uintN length = 0;
    if (flags & JSREG_GLOB) chars[length++] = 'g';
    if (flags & JSREG_FOLD) chars[length++] = 'i';
    if (flags & JSREG_MULTILINE) chars[length++] = 'm';
    if (flags & JSREG_UNICODE) chars[length++] = 'u';
    if (flags & JSREG_STICKY) chars[length++] = 'y';
    return JS_NewUCStringCopyN(cx, chars, length);
}

static JSBool
InitializeModernRegExp(JSContext *cx, JSObject *obj, jsval pattern, jsval flags,
                       jsval *rval)
{
    jsval values[3] = {pattern, flags, OBJECT_TO_JSVAL(obj)};
    JSTempValueRooter root;
    JSString *str, *flagsString;
    JSRegExp *re;
    JSBool ok = JS_FALSE;
    JS_PUSH_TEMP_ROOT(cx, 3, values, &root);
    if (!JS_DefinePropertyWithTinyId(cx, obj, "lastIndex", REGEXP_LAST_INDEX,
                                     JSVAL_VOID, regexp_getProperty,
                                     regexp_setProperty, JSPROP_PERMANENT)) goto out;
    str = JSVAL_IS_VOID(values[0]) ? cx->runtime->emptyString : js_ValueToString(cx, values[0]);
    if (!str) goto out;
    values[0] = STRING_TO_JSVAL(str);
    flagsString = JSVAL_IS_VOID(values[1]) ? cx->runtime->emptyString : js_ValueToString(cx, values[1]);
    if (!flagsString) goto out;
    values[1] = STRING_TO_JSVAL(flagsString);
    re = NewRegExpOpt(cx, NULL, str, flagsString, JS_FALSE, JS_TRUE);
    if (!re) goto out;
    if (!JS_SetPrivate(cx, obj, re)) { js_DestroyRegExp(cx, re); goto out; }
    if (!SetRegExpIndexValue(cx, obj, JSVAL_ZERO)) goto out;
    *rval = values[2]; ok = JS_TRUE;
  out:
    JS_POP_TEMP_ROOT(cx, &root);
    return ok;
}

/* RegExpCreate initializes a pattern directly; it does not perform the
 * constructor's IsRegExp/identity/slot-copying steps. */
JSBool
js_RegExpCreate(JSContext *cx, JSObject *global, jsval pattern, jsval flags,
                 jsval *rval)
{
    jsval values[3] = {pattern, flags, OBJECT_TO_JSVAL(global)};
    JSTempValueRooter root;
    JSObject *proto, *obj;
    JSBool ok = JS_FALSE;
    JS_PUSH_TEMP_ROOT(cx, 3, values, &root);
    proto = js_BuiltinPrototype(cx, global, JSProto_RegExp);
    if (!proto) goto out;
    obj = js_NewObject(cx, &js_RegExpClass, proto, global);
    if (obj) ok = InitializeModernRegExp(cx, obj, values[0], values[1], rval);
  out:
    JS_POP_TEMP_ROOT(cx, &root);
    return ok;
}

JSBool
js_ModernRegExpConstructor(JSContext *cx, JSObject *obj, uintN argc,
                           jsval *argv, jsval *rval)
{
    jsval values[4] = {JSVAL_VOID, JSVAL_VOID, JSVAL_VOID, JSVAL_VOID};
    JSTempValueRooter root;
    JSBool isRegExp, constructing = (cx->fp->flags & JSFRAME_CONSTRUCTING) != 0;
    JSBool ok = JS_FALSE;
    JSObject *newTarget, *pattern, *global, *proto;
    JSRegExp *re;
    JSString *flagsString;
    uintN flags;
    JS_PUSH_TEMP_ROOT(cx, 4, values, &root);
    if (!js_IsRegExp(cx, argv[0], &isRegExp)) goto out;
    newTarget = constructing && cx->fp->newTarget
                ? cx->fp->newTarget : JSVAL_TO_OBJECT(argv[-2]);
    if (!constructing && isRegExp && JSVAL_IS_VOID(argv[1])) {
        if (!JS_GetProperty(cx, JSVAL_TO_OBJECT(argv[0]), "constructor", &values[2])) goto out;
        if (values[2] == OBJECT_TO_JSVAL(newTarget)) {
            *rval = argv[0]; ok = JS_TRUE; goto out;
        }
    }
    values[1] = argv[1];
    if (!JSVAL_IS_PRIMITIVE(argv[0]) &&
        OBJ_GET_CLASS(cx, JSVAL_TO_OBJECT(argv[0])) == &js_RegExpClass) {
        pattern = JSVAL_TO_OBJECT(argv[0]);
        JS_LOCK_OBJ(cx, pattern);
        re = (JSRegExp *)JS_GetPrivate(cx, pattern);
        if (!re) {
            JS_UNLOCK_OBJ(cx, pattern);
            RegExpReceiverError(cx, "constructor"); goto out;
        }
        values[0] = STRING_TO_JSVAL(re->source); flags = re->flags;
        JS_UNLOCK_OBJ(cx, pattern);
        if (JSVAL_IS_VOID(argv[1])) {
            flagsString = RegExpFlagsString(cx, flags);
            if (!flagsString) goto out;
            values[1] = STRING_TO_JSVAL(flagsString);
        }
    } else if (isRegExp) {
        pattern = JSVAL_TO_OBJECT(argv[0]);
        if (!JS_GetProperty(cx, pattern, "source", &values[0])) goto out;
        if (JSVAL_IS_VOID(argv[1]) && !JS_GetProperty(cx, pattern, "flags", &values[1])) goto out;
    } else values[0] = argv[0];
    if (!OBJ_GET_PROPERTY(cx, newTarget,
                          ATOM_TO_JSID(cx->runtime->atomState.classPrototypeAtom), &values[2])) goto out;
    global = js_BuiltinGlobal(cx, argv);
    if (JSVAL_IS_PRIMITIVE(values[2])) {
        global = js_ConstructorGlobal(cx, newTarget);
        if (!global) goto out;
        proto = js_BuiltinPrototype(cx, global, JSProto_RegExp);
        if (!proto) goto out;
        values[2] = OBJECT_TO_JSVAL(proto);
    } else proto = JSVAL_TO_OBJECT(values[2]);
    obj = js_NewObject(cx, &js_RegExpClass, proto, global);
    if (!obj) goto out;
    values[3] = OBJECT_TO_JSVAL(obj);
    ok = InitializeModernRegExp(cx, obj, values[0], values[1], rval);
  out:
    JS_POP_TEMP_ROOT(cx, &root);
    return ok;
}

static JSBool
RegExpSpecies(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    *rval = argv[-1]; return JS_TRUE;
}

static JSBool
InitRegExpSpecies(JSContext *cx, JSObject *global, JSObject *ctor)
{
    jsid id;
    JSFunction *fun;
    JSTempValueRooter root;
    JSBool ok;
    if (!js_WellKnownSymbolId(cx, JS_WKS_SPECIES, &id)) return JS_FALSE;
    fun = JS_NewFunction(cx, RegExpSpecies, 0, JSFUN_STRICT | JSFUN_NO_CONSTRUCT,
                         global, "get [Symbol.species]");
    if (!fun) return JS_FALSE;
    JS_PUSH_TEMP_ROOT_OBJECT(cx, fun->object, &root);
    ok = OBJ_DEFINE_PROPERTY(cx, ctor, id, JSVAL_VOID, (JSPropertyOp)fun->object,
                             NULL, JSPROP_GETTER | JSPROP_SHARED, NULL);
    JS_POP_TEMP_ROOT(cx, &root);
    return ok;
}

JSObject *
js_InitRegExpClass(JSContext *cx, JSObject *obj)
{
    JSObject *proto, *ctor;
    jsval rval;
    /* Lazy initialization can be triggered by a script of another edition.
     * Follow the global's established policy, not that script's caller. */
    JSBool modern = js_GetCachedClassObject(cx, obj, JSProto_Object)
                    ? js_IsModernGlobal(cx, obj) : JS_VERSION_IS_ES2015(cx);
    JSFunction *fun;

    proto = JS_InitClass(cx, obj, NULL,
                         modern ? &regexpPrototypeClass : &js_RegExpClass,
                         modern ? js_ModernRegExpConstructor : RegExp, 2,
                         modern ? NULL : regexp_props, regexp_methods,
                         regexp_static_props, NULL);

    if (!proto || !(ctor = JS_GetConstructor(cx, proto)))
        return NULL;
    if (!js_SetBuiltinMethodFlags(cx, proto, regexp_methods,
                                  JSFUN_NO_CONSTRUCT | JSFUN_REQUIRE_THIS))
        return NULL;
    if (!JS_AliasProperty(cx, ctor, "input",        "$_") ||
        !JS_AliasProperty(cx, ctor, "multiline",    "$*") ||
        !JS_AliasProperty(cx, ctor, "lastMatch",    "$&") ||
        !JS_AliasProperty(cx, ctor, "lastParen",    "$+") ||
        !JS_AliasProperty(cx, ctor, "leftContext",  "$`") ||
        !JS_AliasProperty(cx, ctor, "rightContext", "$'")) {
        goto bad;
    }

    if (modern) {
        if (!JS_DefineFunction(cx, proto, "compile", regexp_compile, 2,
                               JSFUN_NO_CONSTRUCT | JSFUN_REQUIRE_THIS))
            goto bad;
        fun = (JSFunction *)JS_GetPrivate(cx, ctor);
        fun->clasp = &js_RegExpClass;
        if (!InitRegExpAccessors(cx, obj, proto) ||
            !JS_GetProperty(cx, proto, js_toString_str, &rval)) goto bad;
        fun = (JSFunction *)JS_GetPrivate(cx, JSVAL_TO_OBJECT(rval));
        fun->flags |= JSFUN_STRICT;
        if (!InitRegExpProtocols(cx, obj, proto) || !InitRegExpSpecies(cx, obj, ctor)) goto bad;
        return proto;
    }
    /* Give legacy RegExp.prototype private data so it matches the empty string. */
    if (!regexp_compile(cx, proto, 0, NULL, &rval))
        goto bad;
    return proto;

bad:
    JS_DeleteProperty(cx, obj, js_RegExpClass.name);
    return NULL;
}

JSObject *
js_NewRegExpObject(JSContext *cx, JSTokenStream *ts,
                   jschar *chars, size_t length, uintN flags)
{
    JSString *str;
    JSObject *obj;
    JSRegExp *re;
    JSTempValueRooter tvr;

    str = js_NewStringCopyN(cx, chars, length, 0);
    if (!str)
        return NULL;
    JS_PUSH_TEMP_ROOT_STRING(cx, str, &tvr);
    re = js_NewRegExp(cx, ts,  str, flags, JS_FALSE);
    if (!re) {
        JS_POP_TEMP_ROOT(cx, &tvr);
        return NULL;
    }
    obj = NewRegExpInstance(cx, NULL);
    if (!obj || !JS_SetPrivate(cx, obj, re)) {
        js_DestroyRegExp(cx, re);
        obj = NULL;
    }
    if (obj && !js_SetLastIndex(cx, obj, 0))
        obj = NULL;
    JS_POP_TEMP_ROOT(cx, &tvr);
    return obj;
}

JSObject *
js_CloneRegExpObject(JSContext *cx, JSObject *obj, JSObject *parent)
{
    JSObject *clone;
    JSRegExp *re;
    JSTempValueRooter root;

    JS_ASSERT(OBJ_GET_CLASS(cx, obj) == &js_RegExpClass);
    clone = NewRegExpInstance(cx, parent);
    if (!clone)
        return NULL;
    JS_PUSH_TEMP_ROOT_OBJECT(cx, clone, &root);
    re = JS_GetPrivate(cx, obj);
    /* Once installed, the clone owns this reference even if subsequent
     * initialization fails and the clone is later finalized. */
    HOLD_REGEXP(cx, re);
    if (!JS_SetPrivate(cx, clone, re)) {
        DROP_REGEXP(cx, re);
        clone = NULL;
    } else if (!js_SetLastIndex(cx, clone, 0)) {
        clone = NULL;
    }
    JS_POP_TEMP_ROOT(cx, &root);
    return clone;
}

JSBool
js_GetLastIndex(JSContext *cx, JSObject *obj, jsdouble *lastIndex)
{
    jsval v;
    JSTempValueRooter root;
    JSBool ok;
    if (!JS_GetReservedSlot(cx, obj, 0, &v)) return JS_FALSE;
    /* Conversion may replace lastIndex and run a second user method. */
    JS_PUSH_TEMP_ROOT(cx, 1, &v, &root);
    ok = js_ValueToNumber(cx, v, lastIndex);
    JS_POP_TEMP_ROOT(cx, &root);
    if (!ok) return JS_FALSE;
    *lastIndex = js_DoubleToInteger(*lastIndex);
    return JS_TRUE;
}

JSBool
js_SetLastIndex(JSContext *cx, JSObject *obj, jsdouble lastIndex)
{
    jsval v;
    uintN attrs;
    JSBool found, own;
    JSObject *global = obj, *parent;
    JSObject *owner;
    JSProperty *prop;
    JSAtom *atom;
    while ((parent = OBJ_GET_PARENT(cx, global)) != NULL) global = parent;
    if (js_IsModernGlobal(cx, global)) {
        atom = js_Atomize(cx, "lastIndex", 9, 0);
        if (!atom || !js_LookupOwnProperty(cx, obj, ATOM_TO_JSID(atom), &owner, &prop))
            return JS_FALSE;
        own = prop != NULL && owner == obj;
        if (prop) OBJ_DROP_PROPERTY(cx, owner, prop);
        if (!own && !JS_DefinePropertyWithTinyId(cx, obj, "lastIndex", REGEXP_LAST_INDEX,
                                               JSVAL_ZERO, regexp_getProperty,
                                               regexp_setProperty, JSPROP_PERMANENT))
            return JS_FALSE;
    }
    if (!JS_GetPropertyAttributes(cx, obj, "lastIndex", &attrs, &found))
        return JS_FALSE;
    if (found && (attrs & JSPROP_READONLY)) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_BAD_DESCRIPTOR);
        return JS_FALSE;
    }
    return js_NewNumberValue(cx, lastIndex, &v) &&
           JS_SetReservedSlot(cx, obj, 0, v);
}
