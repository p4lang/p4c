/**
 * Copyright (C) 2024 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
 * except in compliance with the License.  You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed under the
 * License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied.  See the License for the specific language governing permissions
 * and limitations under the License.
 *
 *
 * SPDX-License-Identifier: Apache-2.0
 */


%x LINESTART LINE1 LINE2 LINE3 COMMENT
%s NORMAL

%{
#include <assert.h>
#include <limits.h>
#include <stack>
static std::stack<int>  indent;
static int              parens=0;
static int indent_depth(const char *);
static int parse_num(YYSTYPE *, const char *s, int base);
static int parse_match(YYSTYPE *, const char *s, int bits_per_digit);

#if YYDEBUG
#undef BEGIN
/* DANGER -- the depends on the internals of how flex sets states, but as
 * DANGER -- its only for debugging, its not too bad */
#define BEGIN(S)  ((yy_start) = 1 + 2*(S), \
                   yydebug ? fprintf(stderr, "Setting lexer state "#S"\n") : 0)
#define DB(...) fprintf(stderr, __VA_ARGS__)
#else
#define DB(...)
#endif

#pragma clang diagnostic ignored "-Wnull-conversion"

%}

ID [A-Za-z_@$]([-.]?*[A-Za-z0-9_@$])*
STR \"(\\.|[^\n"\\])*\"
%option nounput noyywrap

%%

<INITIAL>.*|\n        { yyless(0); BEGIN(LINESTART); indent.push(0); }
<LINESTART>[ \t]*     { int depth = indent_depth(yytext);
                        if (depth < indent.top()) {
                            indent.pop();
                            yyless(0);
                            return UNINDENT; }
                        BEGIN(NORMAL);
                        if (depth > indent.top()) {
                            indent.push(depth);
                            return INDENT; } }

<LINESTART>"#line"    { BEGIN(LINE1); }
<LINESTART>"# "       { BEGIN(LINE1); }
<LINE1>[0-9]+         { line_file_map[lineno].second = atoi(yytext)-1;
                        DB("next line is %s\n", yytext);
                        BEGIN(LINE2); }
<LINE2>\"[^"]*        { line_file_map[lineno].first = yytext+1;
                        DB("file is '%s'\n", yytext+1);
                        BEGIN(LINE3); }
<LINE1,LINE2>[ \t]      ;
<LINE1,LINE2>.        { BEGIN(LINE3); }
<LINE3>.                ;
<LINE1,LINE2,LINE3>\n { lineno++; BEGIN(LINESTART); }
<LINE1,LINE2,LINE3><<EOF>> { BEGIN(LINESTART); }

<LINESTART>[ \t]*"#"  { BEGIN(LINE3); }
<LINESTART>.          { yyless(0);
                        if (indent.top() > 0) {
                            indent.pop();
                            return UNINDENT; }
                        BEGIN(NORMAL); }
<LINESTART><<EOF>>    { if (indent.top() > 0) {
                            indent.pop();
                            return UNINDENT; }
                        BEGIN(NORMAL); }
<LINESTART>[ \t\r]*\n { lineno++; }

[[({]             { parens++; return *yytext; }
[])}]             { if (--parens < 0) parens = 0;
                    return *yytext; }
\n                { lineno++;
                    if (parens == 0) {
                        BEGIN(LINESTART);
                        return '\n'; } }
[ \t\r]+            ;
".."              { return DOTDOT; }
{ID}              { yylval.str = strdup(yytext); return ID; }
{STR}             { yylval.str = strndup(yytext+1, yyleng-2); return STR; }
[0-9_]+           { return parse_num(&yylval, yytext, 10); }
0[xX][0-9a-fA-F_]+ { return parse_num(&yylval, yytext+2, 16); }
0[oO][0-7_]+      { return parse_num(&yylval, yytext+2, 8); }
0[bB][0-1_]+      { return parse_num(&yylval, yytext+2, 2); }
0[xX][0-9a-fA-F*_]+ { return parse_match(&yylval, yytext+2, 4); }
0[oO][0-7*_]+     { return parse_match(&yylval, yytext+2, 3); }
0[bB][0-1*_]+     { return parse_match(&yylval, yytext+2, 1); }
"*"               { return parse_match(&yylval, yytext, 0); }

"#".*               ;
"/*"              { BEGIN(COMMENT); }
<COMMENT>"*/"     { BEGIN(NORMAL); }
<COMMENT>.              ;
<COMMENT>\n       { lineno++; }

.                 { return *yytext; }


%%

/* flex's #line generation is broken, so we manually resync so we can debug */
#line 104 "lex-yaml.l"
int indent_depth(const char *pfx) {
    int rv = 0;
    while (*pfx)
        switch(*pfx++) {
        case ' ': rv++; break;
        case '\t': rv &= ~7; rv += 8; break;
        default:
            return rv;}
    return rv;
}

#include "gen/uptr_sizes.h"

void bigint_mul(VECTOR(uintptr_t) &val, unsigned f) {
    unsigned carry = 0;
    for (int i = 0; i < val.size; i++) {
#if defined(uint2ptr_t)
        uint2ptr_t v = val.data[i];
        v = v * f + carry;
        val.data[i] = (uintptr_t)v;
        carry = v >> CHAR_BIT * sizeof(uintptr_t);
#elif defined(uinthptr_t)
        uinthptr_t lo = val.data[i],
                   hi = val.data[i] >> CHAR_BIT * sizeof(uinthptr_t);
        uintptr_t tmp = (uintptr_t)lo * f + carry;
        lo = tmp;
        tmp >>= CHAR_BIT * sizeof(uinthptr_t);
        tmp += (uintptr_t)hi * f;
        carry = tmp >> CHAR_BIT * sizeof(uinthptr_t);
        val.data[i] = (tmp << (CHAR_BIT * sizeof(uinthptr_t))) + lo;
#else
#error "No appropriately sized type for bigint_mul"
#endif
    }
    if (carry)
        VECTOR_add(val, carry);
}

void bigint_add(VECTOR(uintptr_t) &val, unsigned a) {
    for (int i = 0; i < val.size; i++) {
        if ((val.data[i] += a) >= a)
            return;
        a = 1; }
    VECTOR_add(val, a);
}

void bigint_init(VECTOR(uintptr_t) &val, int64_t v) {
    if (sizeof(int64_t)/sizeof(uintptr_t) > 1) {
        VECTOR_init(val, sizeof(int64_t)/sizeof(uintptr_t));
        do {
            val.data[val.size++] = v;
            v >>= CHAR_BIT * sizeof(uintptr_t) / 2;
            v >>= CHAR_BIT * sizeof(uintptr_t) / 2;
        } while (v > 0);
    } else {
        VECTOR_init1(val, v);
    }
}

int parse_num(YYSTYPE *val, const char *s, int base) {
    int rv = INT;
    val->i = 0;
    s--;
    while (*++s) {
        if (*s == '_') continue;
        /* The comparison is intentionally against LONG_MAX, so that we put values larger than
           uintptr_t size in big ints.
        */
        if (rv == INT && val->i > INT64_MAX/base) {
            bigint_init(val->bigi, val->i);
            rv = BIGINT; }
        if (rv == INT)
            val->i *= base;
        else
            bigint_mul(val->bigi, base);
        switch (*s) {
        case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9':
            if (rv == INT)
                val->i += *s - '0';
            else
                bigint_add(val->bigi, *s - '0');
            break;
        case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
            if (rv == INT)
                val->i += *s - 'a' + 10;
            else
                bigint_add(val->bigi, *s - 'a' + 10);
            break;
        case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
            if (rv == INT)
                val->i += *s - 'A' + 10;
            else
                bigint_add(val->bigi, *s - 'A' + 10);
            break;
        default:
            assert(0); }
        if (rv == INT && val->i > 0xffffffff) {
            // We limit INT tokens to what will fit in 32 bits (unsigned) even though
            // we use a int64_t to hold them, as most parts of the compiler can't deal
            // with larger constants.  The few places that can deal with >32bit values
            // handle BIGINTs
            bigint_init(val->bigi, val->i);
            rv = BIGINT; } }
    return rv;
}

int parse_match(YYSTYPE *val, const char *s, int bits_per_digit) {
    val->match.word0 = val->match.word1 = 0;
    int rv = MATCH;
    VECTOR(match_t) bigm = EMPTY_VECTOR_INIT;
    if (*s == '*' && bits_per_digit == 0) return rv;
    unsigned digit = 0, digit_mask = (1U << bits_per_digit) - 1;
    decltype(val->match.word0) overflow_mask = digit_mask;
    overflow_mask <<= sizeof(val->match.word0) * 8 - bits_per_digit;
    s--;
    while (*++s) {
        if (*s == '_') continue;
        if (rv == BIGMATCH || ((val->match.word0 | val->match.word1) & overflow_mask)) {
            rv = BIGMATCH;
            if (bigm.size < 2) {
                VECTOR_resize(bigm, 2);
                bigm.data[0].word0 = bigm.data[0].word1 = 
                bigm.data[1].word0 = bigm.data[1].word1 = 0; }
            if ((bigm.data[bigm.size-1].word0 | bigm.data[bigm.size-1].word1) & overflow_mask) {
                VECTOR_resize(bigm, bigm.size+1);
                bigm.data[bigm.size-1].word0 = bigm.data[bigm.size-1].word1 = 0; }
            for (int i = bigm.size-1; i > 0; --i) {
                bigm.data[i].word0 <<= bits_per_digit;
                bigm.data[i].word0 |= bigm.data[i].word0 >> (64 - bits_per_digit);
                bigm.data[i].word1 <<= bits_per_digit;
                bigm.data[i].word1 |= bigm.data[i].word1 >> (64 - bits_per_digit); }
            bigm.data[1].word0 |= val->match.word0 >> (64 - bits_per_digit);
            bigm.data[1].word1 |= val->match.word1 >> (64 - bits_per_digit); }
        val->match.word0 <<= bits_per_digit;
        val->match.word1 <<= bits_per_digit;
        switch (*s) {
        case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9':
            digit = *s - '0';
            break;
        case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
            digit = *s - 'a' + 10;
            break;
        case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
            digit = *s - 'A' + 10;
            break;
        case '*':
            val->match.word1 |= digit_mask;
            digit = 0;
            break;
        default:
            assert(0); }
        assert((digit & ~digit_mask) == 0);
        val->match.word1 |= digit;
        val->match.word0 |= digit_mask & ~digit; }
    if (rv == BIGMATCH) {
        bigm.data[0] = val->match;
        val->bigm = bigm; }
    return rv;
}
