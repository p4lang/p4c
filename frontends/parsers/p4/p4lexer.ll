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

[ \t\r]+          ;
[\n]            { BEGIN INITIAL; }
"//".*            ;
"/*"            { BEGIN COMMENT; }
<COMMENT>"*/"   { BEGIN NORMAL; }
<COMMENT>.        ;
<COMMENT>[\n]     ;

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

"abstract"      { BEGIN(NORMAL); return Parser::make_ABSTRACT(driver.yylloc); }
"action"        { BEGIN(NORMAL); return Parser::make_ACTION(driver.yylloc); }
"actions"       { BEGIN(NORMAL); return Parser::make_ACTIONS(driver.yylloc); }
"apply"         { BEGIN(NORMAL); return Parser::make_APPLY(driver.yylloc); }
"bool"          { BEGIN(NORMAL); return Parser::make_BOOL(driver.yylloc); }
"bit"           { BEGIN(NORMAL); return Parser::make_BIT(driver.yylloc); }
"const"         { BEGIN(NORMAL); return Parser::make_CONST(driver.yylloc); }
"control"       { BEGIN(NORMAL); return Parser::make_CONTROL(driver.yylloc); }
"default"       { BEGIN(NORMAL); return Parser::make_DEFAULT(driver.yylloc); }
"else"          { BEGIN(NORMAL); return Parser::make_ELSE(driver.yylloc); }
"entries"       { BEGIN(NORMAL); return Parser::make_ENTRIES(driver.yylloc); }
"enum"          { BEGIN(NORMAL); return Parser::make_ENUM(driver.yylloc); }
"error"         { BEGIN(NORMAL); return Parser::make_ERROR(driver.yylloc); }
"exit"          { BEGIN(NORMAL); return Parser::make_EXIT(driver.yylloc); }
"extern"        { BEGIN(NORMAL); return Parser::make_EXTERN(driver.yylloc); }
"false"         { BEGIN(NORMAL); return Parser::make_FALSE(driver.yylloc); }
"header"        { BEGIN(NORMAL); return Parser::make_HEADER(driver.yylloc); }
"header_union"  { BEGIN(NORMAL); return Parser::make_HEADER_UNION(driver.yylloc); }
"if"            { BEGIN(NORMAL); return Parser::make_IF(driver.yylloc); }
"in"            { BEGIN(NORMAL); return Parser::make_IN(driver.yylloc); }
"inout"         { BEGIN(NORMAL); return Parser::make_INOUT(driver.yylloc); }
"int"           { BEGIN(NORMAL); return Parser::make_INT(driver.yylloc); }
"key"           { BEGIN(NORMAL); return Parser::make_KEY(driver.yylloc); }
"match_kind"    { BEGIN(NORMAL); return Parser::make_MATCH_KIND(driver.yylloc); }
"out"           { BEGIN(NORMAL); return Parser::make_OUT(driver.yylloc); }
"parser"        { BEGIN(NORMAL); return Parser::make_PARSER(driver.yylloc); }
"package"       { BEGIN(NORMAL); return Parser::make_PACKAGE(driver.yylloc); }
"return"        { BEGIN(NORMAL); return Parser::make_RETURN(driver.yylloc); }
"select"        { BEGIN(NORMAL); return Parser::make_SELECT(driver.yylloc); }
"state"         { BEGIN(NORMAL); return Parser::make_STATE(driver.yylloc); }
"struct"        { BEGIN(NORMAL); return Parser::make_STRUCT(driver.yylloc); }
"switch"        { BEGIN(NORMAL); return Parser::make_SWITCH(driver.yylloc); }
"table"         { BEGIN(NORMAL); return Parser::make_TABLE(driver.yylloc); }
"this"          { BEGIN(NORMAL); return Parser::make_THIS(driver.yylloc); }
"transition"    { BEGIN(NORMAL); return Parser::make_TRANSITION(driver.yylloc); }
"true"          { BEGIN(NORMAL); return Parser::make_TRUE(driver.yylloc); }
"tuple"         { BEGIN(NORMAL); return Parser::make_TUPLE(driver.yylloc); }
"typedef"       { BEGIN(NORMAL); return Parser::make_TYPEDEF(driver.yylloc); }
"varbit"        { BEGIN(NORMAL); return Parser::make_VARBIT(driver.yylloc); }
"value_set"     { BEGIN(NORMAL); return Parser::make_VALUESET(driver.yylloc); }
"void"          { BEGIN(NORMAL); return Parser::make_VOID(driver.yylloc); }
"_"             { BEGIN(NORMAL); return Parser::make_DONTCARE(driver.yylloc); }
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
                      return Parser::make_TYPE(name, driver.yylloc);
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

"&&&"           { BEGIN(NORMAL); return Parser::make_MASK(driver.yylloc); }
".."            { BEGIN(NORMAL); return Parser::make_RANGE(driver.yylloc); }
"<<"            { BEGIN(NORMAL); return Parser::make_SHL(driver.yylloc); }
"&&"            { BEGIN(NORMAL); return Parser::make_AND(driver.yylloc); }
"||"            { BEGIN(NORMAL); return Parser::make_OR(driver.yylloc); }
"=="            { BEGIN(NORMAL); return Parser::make_EQ(driver.yylloc); }
"!="            { BEGIN(NORMAL); return Parser::make_NE(driver.yylloc); }
">="            { BEGIN(NORMAL); return Parser::make_GE(driver.yylloc); }
"<="            { BEGIN(NORMAL); return Parser::make_LE(driver.yylloc); }
"++"            { BEGIN(NORMAL); return Parser::make_PP(driver.yylloc); }

"+"            { BEGIN(NORMAL); return Parser::make_PLUS(driver.yylloc); }
"-"            { BEGIN(NORMAL); return Parser::make_MINUS(driver.yylloc); }
"*"            { BEGIN(NORMAL); return Parser::make_MUL(driver.yylloc); }
"/"            { BEGIN(NORMAL); return Parser::make_DIV(driver.yylloc); }
"%"            { BEGIN(NORMAL); return Parser::make_MOD(driver.yylloc); }

"|"            { BEGIN(NORMAL); return Parser::make_BIT_OR(driver.yylloc); }
"&"            { BEGIN(NORMAL); return Parser::make_BIT_AND(driver.yylloc); }
"^"            { BEGIN(NORMAL); return Parser::make_BIT_XOR(driver.yylloc); }
"~"            { BEGIN(NORMAL); return Parser::make_COMPLEMENT(driver.yylloc); }

"("            { BEGIN(NORMAL); return Parser::make_L_PAREN(driver.yylloc); }
")"            { BEGIN(NORMAL); return Parser::make_R_PAREN(driver.yylloc); }
"["            { BEGIN(NORMAL); return Parser::make_L_BRACKET(driver.yylloc); }
"]"            { BEGIN(NORMAL); return Parser::make_R_BRACKET(driver.yylloc); }
"{"            { BEGIN(NORMAL); return Parser::make_L_BRACE(driver.yylloc); }
"}"            { BEGIN(NORMAL); return Parser::make_R_BRACE(driver.yylloc); }
"<"            { BEGIN(NORMAL); return Parser::make_L_ANGLE(driver.yylloc); }
">"            { BEGIN(NORMAL); return Parser::make_R_ANGLE(driver.yylloc); }

"!"            { BEGIN(NORMAL); return Parser::make_NOT(driver.yylloc); }
":"            { BEGIN(NORMAL); return Parser::make_COLON(driver.yylloc); }
","            { BEGIN(NORMAL); return Parser::make_COMMA(driver.yylloc); }
"?"            { BEGIN(NORMAL); return Parser::make_QUESTION(driver.yylloc); }
"."            { BEGIN(NORMAL); return Parser::make_DOT(driver.yylloc); }
"="            { BEGIN(NORMAL); return Parser::make_ASSIGN(driver.yylloc); }
";"            { BEGIN(NORMAL); return Parser::make_SEMICOLON(driver.yylloc); }
"@"            { BEGIN(NORMAL); return Parser::make_AT(driver.yylloc); }

.              { return Parser::make_UNEXPECTED_TOKEN(driver.yylloc); }

%%
