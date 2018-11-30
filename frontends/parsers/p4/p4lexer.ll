%{
#include "frontends/common/constantParsing.h"
#include "frontends/parsers/parserDriver.h"
#include "frontends/parsers/p4/p4lexer.hpp"
#include "frontends/parsers/p4/p4parser.hpp"

using Parser = P4::P4Parser;

#undef  YY_DECL
#define YY_DECL Parser::symbol_type P4::P4Lexer::yylex(P4::P4ParserDriver& driver)

#define YY_USER_ACTION driver.onReadToken(yytext);
#define yyterminate() return Parser::make_END(driver.yylloc);

#define makeToken(symbol) \
    Parser::make_ ## symbol( \
        P4::Token(Parser::token::TOK_ ## symbol, yytext), \
        driver.yylloc)

// Silence the warnings triggered by the code flex generates.
#pragma GCC diagnostic ignored "-Wpragmas"
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wtautological-undefined-compare"
#ifdef __clang__
#pragma clang diagnostic ignored "-Wnull-conversion"
#endif

%}

%option c++
%option outfile="lex.yy.c"
%option yyclass="P4::P4Lexer"
%option prefix="p4"
%option nodefault noyywrap nounput noinput noyyget_leng
%option noyyget_debug noyyset_debug noyyget_extra noyyset_extra noyyget_in noyyset_in
%option noyyget_out noyyset_out noyyget_text noyyget_lineno noyyset_lineno

%x COMMENT STRING
%x LINE1 LINE2 LINE3
%s NORMAL


%%

%{
    // Insert a token at the beginning of the stream to tell the parser that
    // the stream should be parsed as a program.
    if (needStartToken) {
        needStartToken = false;
        return Parser::make_START_PROGRAM(driver.yylloc);
    }
%}

[ \t\r]+              ;
[\n]                  { BEGIN INITIAL; }
"//".*                { driver.onReadComment(yytext+2, true); }
"/*"                  { BEGIN COMMENT; }
<COMMENT>([^*]|[*]+[^/*])*[*]+"/" {
                         /* http://www.cs.dartmouth.edu/~mckeeman/cs118/assignments/comment.html */
                         driver.onReadComment(yytext, false); BEGIN NORMAL; }

<INITIAL>"#line"      { BEGIN(LINE1); }
<INITIAL>"# "         { BEGIN(LINE1); }
<INITIAL>[ \t]*"#"    { BEGIN(LINE3); }
<LINE1>[0-9]+         { BEGIN(LINE2); driver.onReadLineNumber(yytext); }
<LINE2>\"[^\"]*       { BEGIN(LINE3); driver.onReadFileName(yytext+1); }
<LINE1,LINE2>[ \t]      ;
<LINE1,LINE2>.        { BEGIN(LINE3); }
<LINE3>.                ;
<LINE1,LINE2,LINE3>\n { BEGIN(INITIAL); }
<LINE1,LINE2,LINE3,COMMENT,NORMAL><<EOF>> { BEGIN(INITIAL); }

\"              { BEGIN(STRING); driver.stringLiteral = ""; }
<STRING>\\\"    { driver.stringLiteral += yytext; }
<STRING>\\\\    { driver.stringLiteral += yytext; }
<STRING>\"      { BEGIN(INITIAL);
                  auto string = cstring(driver.stringLiteral);
                  return Parser::make_STRING_LITERAL(string, driver.yylloc); }
<STRING>.       { driver.stringLiteral += yytext; }
<STRING>\n      { driver.stringLiteral += yytext; }

"abstract"      { BEGIN(NORMAL); return makeToken(ABSTRACT); }
"action"        { BEGIN(NORMAL); return makeToken(ACTION); }
"actions"       { BEGIN(NORMAL); return makeToken(ACTIONS); }
"apply"         { BEGIN(NORMAL); return makeToken(APPLY); }
"bool"          { BEGIN(NORMAL); return makeToken(BOOL); }
"bit"           { BEGIN(NORMAL); return makeToken(BIT); }
"const"         { BEGIN(NORMAL); return makeToken(CONST); }
"control"       { BEGIN(NORMAL); return makeToken(CONTROL); }
"default"       { BEGIN(NORMAL); return makeToken(DEFAULT); }
"else"          { BEGIN(NORMAL); return makeToken(ELSE); }
"entries"       { BEGIN(NORMAL); return makeToken(ENTRIES); }
"enum"          { BEGIN(NORMAL); return makeToken(ENUM); }
"error"         { BEGIN(NORMAL); return makeToken(ERROR); }
"exit"          { BEGIN(NORMAL); return makeToken(EXIT); }
"extern"        { BEGIN(NORMAL); return makeToken(EXTERN); }
"false"         { BEGIN(NORMAL); return makeToken(FALSE); }
"header"        { BEGIN(NORMAL); return makeToken(HEADER); }
"header_union"  { BEGIN(NORMAL); return makeToken(HEADER_UNION); }
"if"            { BEGIN(NORMAL); return makeToken(IF); }
"in"            { BEGIN(NORMAL); return makeToken(IN); }
"inout"         { BEGIN(NORMAL); return makeToken(INOUT); }
"int"           { BEGIN(NORMAL); return makeToken(INT); }
"key"           { BEGIN(NORMAL); return makeToken(KEY); }
"match_kind"    { BEGIN(NORMAL); return makeToken(MATCH_KIND); }
"type"          { BEGIN(NORMAL); return makeToken(TYPE); }
"out"           { BEGIN(NORMAL); return makeToken(OUT); }
"parser"        { BEGIN(NORMAL); return makeToken(PARSER); }
"package"       { BEGIN(NORMAL); return makeToken(PACKAGE); }
"return"        { BEGIN(NORMAL); return makeToken(RETURN); }
"select"        { BEGIN(NORMAL); return makeToken(SELECT); }
"state"         { BEGIN(NORMAL); return makeToken(STATE); }
"struct"        { BEGIN(NORMAL); return makeToken(STRUCT); }
"switch"        { BEGIN(NORMAL); return makeToken(SWITCH); }
"table"         { BEGIN(NORMAL); return makeToken(TABLE); }
"this"          { BEGIN(NORMAL); return makeToken(THIS); }
"transition"    { BEGIN(NORMAL); return makeToken(TRANSITION); }
"true"          { BEGIN(NORMAL); return makeToken(TRUE); }
"tuple"         { BEGIN(NORMAL); return makeToken(TUPLE); }
"typedef"       { BEGIN(NORMAL); return makeToken(TYPEDEF); }
"varbit"        { BEGIN(NORMAL); return makeToken(VARBIT); }
"value_set"     { BEGIN(NORMAL); return makeToken(VALUESET); }
"void"          { BEGIN(NORMAL); return makeToken(VOID); }
"_"             { BEGIN(NORMAL); return makeToken(DONTCARE); }
[A-Za-z_][A-Za-z0-9_]* {
                  BEGIN(NORMAL);
                  cstring name = yytext;
                  Util::ProgramStructure::SymbolKind kind =
                      driver.structure->lookupIdentifier(name);
                  switch (kind)
                  {
                  /* FIXME: if the type is a reserved keyword this doesn't work */
                  case Util::ProgramStructure::SymbolKind::Identifier:
                      driver.onReadIdentifier(name);
                      return Parser::make_IDENTIFIER(name, driver.yylloc);
                  case Util::ProgramStructure::SymbolKind::Type:
                      return Parser::make_TYPE_IDENTIFIER(name, driver.yylloc);
                  default:
                      BUG("Unexpected symbol kind");
                  }
                }

0[xX][0-9a-fA-F_]+ { BEGIN(NORMAL);
                     UnparsedConstant constant{yytext, 2, 16, false};
                     return Parser::make_INTEGER(constant, driver.yylloc); }
0[dD][0-9_]+       { BEGIN(NORMAL);
                     UnparsedConstant constant{yytext, 2, 10, false};
                     return Parser::make_INTEGER(constant, driver.yylloc); }
0[oO][0-7_]+       { BEGIN(NORMAL);
                     UnparsedConstant constant{yytext, 2, 8, false};
                     return Parser::make_INTEGER(constant, driver.yylloc); }
0[bB][01_]+        { BEGIN(NORMAL);
                     UnparsedConstant constant{yytext, 2, 2, false};
                     return Parser::make_INTEGER(constant, driver.yylloc); }
[0-9][0-9_]*       { BEGIN(NORMAL);
                     UnparsedConstant constant{yytext, 0, 10, false};
                     return Parser::make_INTEGER(constant, driver.yylloc); }

[0-9]+[ws]0[xX][0-9a-fA-F_]+ { BEGIN(NORMAL);
                               UnparsedConstant constant{yytext, 2, 16, true};
                               return Parser::make_INTEGER(constant, driver.yylloc); }
[0-9]+[ws]0[dD][0-9_]+  { BEGIN(NORMAL);
                          UnparsedConstant constant{yytext, 2, 10, true};
                          return Parser::make_INTEGER(constant, driver.yylloc); }
[0-9]+[ws]0[oO][0-7_]+  { BEGIN(NORMAL);
                          UnparsedConstant constant{yytext, 2, 8, true};
                          return Parser::make_INTEGER(constant, driver.yylloc); }
[0-9]+[ws]0[bB][01_]+   { BEGIN(NORMAL);
                          UnparsedConstant constant{yytext, 2, 2, true};
                          return Parser::make_INTEGER(constant, driver.yylloc); }
[0-9]+[ws][0-9_]+       { BEGIN(NORMAL);
                          UnparsedConstant constant{yytext, 0, 10, true};
                          return Parser::make_INTEGER(constant, driver.yylloc); }

"&&&"           { BEGIN(NORMAL); return makeToken(MASK); }
".."            { BEGIN(NORMAL); return makeToken(RANGE); }
"<<"            { BEGIN(NORMAL); return makeToken(SHL); }
"&&"            { BEGIN(NORMAL); return makeToken(AND); }
"||"            { BEGIN(NORMAL); return makeToken(OR); }
"=="            { BEGIN(NORMAL); return makeToken(EQ); }
"!="            { BEGIN(NORMAL); return makeToken(NE); }
">="            { BEGIN(NORMAL); return makeToken(GE); }
"<="            { BEGIN(NORMAL); return makeToken(LE); }
"++"            { BEGIN(NORMAL); return makeToken(PP); }

"+"            { BEGIN(NORMAL); return makeToken(PLUS); }
"|+|"          { BEGIN(NORMAL); return makeToken(PLUS_SAT); }
"-"            { BEGIN(NORMAL); return makeToken(MINUS); }
"|-|"          { BEGIN(NORMAL); return makeToken(MINUS_SAT); }
"*"            { BEGIN(NORMAL); return makeToken(MUL); }
"/"            { BEGIN(NORMAL); return makeToken(DIV); }
"%"            { BEGIN(NORMAL); return makeToken(MOD); }

"|"            { BEGIN(NORMAL); return makeToken(BIT_OR); }
"&"            { BEGIN(NORMAL); return makeToken(BIT_AND); }
"^"            { BEGIN(NORMAL); return makeToken(BIT_XOR); }
"~"            { BEGIN(NORMAL); return makeToken(COMPLEMENT); }

"("            { BEGIN(NORMAL); return makeToken(L_PAREN); }
")"            { BEGIN(NORMAL); return makeToken(R_PAREN); }
"["            { BEGIN(NORMAL); return makeToken(L_BRACKET); }
"]"            { BEGIN(NORMAL); return makeToken(R_BRACKET); }
"{"            { BEGIN(NORMAL); return makeToken(L_BRACE); }
"}"            { BEGIN(NORMAL); return makeToken(R_BRACE); }
"<"            { BEGIN(NORMAL); return makeToken(L_ANGLE); }
">"            { BEGIN(NORMAL); return makeToken(R_ANGLE); }

"!"            { BEGIN(NORMAL); return makeToken(NOT); }
":"            { BEGIN(NORMAL); return makeToken(COLON); }
","            { BEGIN(NORMAL); return makeToken(COMMA); }
"?"            { BEGIN(NORMAL); return makeToken(QUESTION); }
"."            { BEGIN(NORMAL); return makeToken(DOT); }
"="            { BEGIN(NORMAL); return makeToken(ASSIGN); }
";"            { BEGIN(NORMAL); return makeToken(SEMICOLON); }
"@"            { BEGIN(NORMAL); return makeToken(AT); }

<*>.|\n        { return makeToken(UNEXPECTED_TOKEN); }

%%
