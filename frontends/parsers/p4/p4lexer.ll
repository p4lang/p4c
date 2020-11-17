%{
#include "frontends/common/constantParsing.h"
#include "frontends/parsers/parserDriver.h"
#include "frontends/parsers/p4/p4lexer.hpp"
#include "frontends/parsers/p4/p4parser.hpp"

using Parser = P4::P4Parser;

#undef  YY_DECL
#define YY_DECL Parser::symbol_type P4::P4Lexer::yylex(P4::P4ParserDriver& driver)

#define YY_USER_ACTION driver.onReadToken(yytext);
#define YY_USER_INIT driver.saveState = NORMAL
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
%s NORMAL PRAGMA_LINE


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
<PRAGMA_LINE>[\n]     { BEGIN INITIAL;
                        driver.saveState = NORMAL;
                        return makeToken(END_PRAGMA); }
[\n]                  { BEGIN INITIAL; }
"//".*                { driver.onReadComment(yytext+2, true); }
"/*"                  { BEGIN COMMENT; }
<COMMENT>([^*]|[*]+[^/*])*[*]+"/" {
                         /* http://www.cs.dartmouth.edu/~mckeeman/cs118/assignments/comment.html */
                         driver.onReadComment(yytext, false);
                         if (driver.saveState == PRAGMA_LINE) {
                             // If the comment contains a newline, end the pragma line.
                             for (int i = 0; i < strlen(yytext); i++) {
                                 if (yytext[i] == '\n') {
                                     driver.saveState = NORMAL;
                                     BEGIN driver.saveState;
                                     return makeToken(END_PRAGMA);
                                 }
                             }
                         }
                         BEGIN driver.saveState; }

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
<STRING>\"      { BEGIN(driver.saveState);
                  driver.template_args = false;
                  auto string = cstring(driver.stringLiteral);
                  return Parser::make_STRING_LITERAL(string, driver.yylloc); }
<STRING>.       { driver.stringLiteral += yytext; }
<STRING>\n      { driver.stringLiteral += yytext; }

"@pragma"       { BEGIN((driver.saveState = PRAGMA_LINE));
                  driver.template_args = false;
                  return makeToken(PRAGMA); }

"abstract"      { BEGIN(driver.saveState); driver.template_args = false;
                  return makeToken(ABSTRACT); }
"action"        { BEGIN(driver.saveState); driver.template_args = false;
                  return makeToken(ACTION); }
"actions"       { BEGIN(driver.saveState); driver.template_args = false;
                  return makeToken(ACTIONS); }
"apply"         { BEGIN(driver.saveState); driver.template_args = false;
                  return makeToken(APPLY); }
"bool"          { BEGIN(driver.saveState); driver.template_args = false;
                  return makeToken(BOOL); }
"bit"           { BEGIN(driver.saveState); driver.template_args = true;
                  return makeToken(BIT); }
"const"         { BEGIN(driver.saveState); driver.template_args = false;
                  return makeToken(CONST); }
"control"       { BEGIN(driver.saveState); driver.template_args = false;
                  return makeToken(CONTROL); }
"default"       { BEGIN(driver.saveState); driver.template_args = false;
                  return makeToken(DEFAULT); }
"else"          { BEGIN(driver.saveState); driver.template_args = false;
                  return makeToken(ELSE); }
"entries"       { BEGIN(driver.saveState); driver.template_args = false;
                  return makeToken(ENTRIES); }
"enum"          { BEGIN(driver.saveState); driver.template_args = false;
                  return makeToken(ENUM); }
"error"         { BEGIN(driver.saveState); driver.template_args = false;
                  return makeToken(ERROR); }
"exit"          { BEGIN(driver.saveState); driver.template_args = false;
                  return makeToken(EXIT); }
"extern"        { BEGIN(driver.saveState); driver.template_args = false;
                  return makeToken(EXTERN); }
"false"         { BEGIN(driver.saveState); driver.template_args = false;
                  return makeToken(FALSE); }
"header"        { BEGIN(driver.saveState); driver.template_args = false;
                  return makeToken(HEADER); }
"header_union"  { BEGIN(driver.saveState); driver.template_args = false;
                  return makeToken(HEADER_UNION); }
"if"            { BEGIN(driver.saveState); driver.template_args = false;
                  return makeToken(IF); }
"in"            { BEGIN(driver.saveState); driver.template_args = false;
                  return makeToken(IN); }
"inout"         { BEGIN(driver.saveState); driver.template_args = false;
                  return makeToken(INOUT); }
"int"           { BEGIN(driver.saveState); driver.template_args = true;
                  return makeToken(INT); }
"key"           { BEGIN(driver.saveState); driver.template_args = false;
                  return makeToken(KEY); }
"match_kind"    { BEGIN(driver.saveState); driver.template_args = false;
                  return makeToken(MATCH_KIND); }
"type"          { BEGIN(driver.saveState); driver.template_args = false;
                  return makeToken(TYPE); }
"out"           { BEGIN(driver.saveState); driver.template_args = false;
                  return makeToken(OUT); }
"parser"        { BEGIN(driver.saveState); driver.template_args = false;
                  return makeToken(PARSER); }
"package"       { BEGIN(driver.saveState); driver.template_args = false;
                  return makeToken(PACKAGE); }
"return"        { BEGIN(driver.saveState); driver.template_args = false;
                  return makeToken(RETURN); }
"select"        { BEGIN(driver.saveState); driver.template_args = false;
                  return makeToken(SELECT); }
"state"         { BEGIN(driver.saveState); driver.template_args = false;
                  return makeToken(STATE); }
"string"        { BEGIN(driver.saveState); driver.template_args = false;
                  return makeToken(STRING); }
"struct"        { BEGIN(driver.saveState); driver.template_args = false;
                  return makeToken(STRUCT); }
"switch"        { BEGIN(driver.saveState); driver.template_args = false;
                  return makeToken(SWITCH); }
"table"         { BEGIN(driver.saveState); driver.template_args = false;
                  return makeToken(TABLE); }
"this"          { BEGIN(driver.saveState); driver.template_args = false;
                  return makeToken(THIS); }
"transition"    { BEGIN(driver.saveState); driver.template_args = false;
                  return makeToken(TRANSITION); }
"true"          { BEGIN(driver.saveState); driver.template_args = false;
                  return makeToken(TRUE); }
"tuple"         { BEGIN(driver.saveState); driver.template_args = true;
                  return makeToken(TUPLE); }
"typedef"       { BEGIN(driver.saveState); driver.template_args = false;
                  return makeToken(TYPEDEF); }
"varbit"        { BEGIN(driver.saveState); driver.template_args = true;
                  return makeToken(VARBIT); }
"value_set"     { BEGIN(driver.saveState); driver.template_args = true;
                  return makeToken(VALUESET); }
"void"          { BEGIN(driver.saveState); driver.template_args = false;
                  return makeToken(VOID); }
"_"             { BEGIN(driver.saveState); driver.template_args = false;
                  return makeToken(DONTCARE); }
[A-Za-z_][A-Za-z0-9_]* {
                  BEGIN(driver.saveState);
                  driver.template_args = false;
                  cstring name = yytext;
                  Util::ProgramStructure::SymbolKind kind =
                      driver.structure->lookupIdentifier(name);
                  switch (kind)
                  {
                  /* FIXME: if the type is a reserved keyword this doesn't work */
                  case Util::ProgramStructure::SymbolKind::TemplateIdentifier:
                      driver.template_args = true;
                      driver.onReadIdentifier(name);
                      return Parser::make_IDENTIFIER(name, driver.yylloc);
                  case Util::ProgramStructure::SymbolKind::Identifier:
                      driver.onReadIdentifier(name);
                      return Parser::make_IDENTIFIER(name, driver.yylloc);
                  case Util::ProgramStructure::SymbolKind::TemplateType:
                      driver.template_args = true;
                      return Parser::make_TYPE_IDENTIFIER(name, driver.yylloc);
                  case Util::ProgramStructure::SymbolKind::Type:
                      return Parser::make_TYPE_IDENTIFIER(name, driver.yylloc);
                  default:
                      BUG("Unexpected symbol kind");
                  }
                }

0[xX][0-9a-fA-F_]+ { BEGIN(driver.saveState);
                     driver.template_args = false;
                     UnparsedConstant constant{yytext, 2, 16, false};
                     return Parser::make_INTEGER(constant, driver.yylloc); }
0[dD][0-9_]+       { BEGIN(driver.saveState);
                     driver.template_args = false;
                     UnparsedConstant constant{yytext, 2, 10, false};
                     return Parser::make_INTEGER(constant, driver.yylloc); }
0[oO][0-7_]+       { BEGIN(driver.saveState);
                     driver.template_args = false;
                     UnparsedConstant constant{yytext, 2, 8, false};
                     return Parser::make_INTEGER(constant, driver.yylloc); }
0[bB][01_]+        { BEGIN(driver.saveState);
                     driver.template_args = false;
                     UnparsedConstant constant{yytext, 2, 2, false};
                     return Parser::make_INTEGER(constant, driver.yylloc); }
[0-9][0-9_]*       { BEGIN(driver.saveState);
                     driver.template_args = false;
                     UnparsedConstant constant{yytext, 0, 10, false};
                     return Parser::make_INTEGER(constant, driver.yylloc); }

[0-9]+[ws]0[xX][0-9a-fA-F_]+ { BEGIN(driver.saveState);
                               driver.template_args = false;
                               UnparsedConstant constant{yytext, 2, 16, true};
                               return Parser::make_INTEGER(constant, driver.yylloc); }
[0-9]+[ws]0[dD][0-9_]+  { BEGIN(driver.saveState);
                          driver.template_args = false;
                          UnparsedConstant constant{yytext, 2, 10, true};
                          return Parser::make_INTEGER(constant, driver.yylloc); }
[0-9]+[ws]0[oO][0-7_]+  { BEGIN(driver.saveState);
                          driver.template_args = false;
                          UnparsedConstant constant{yytext, 2, 8, true};
                          return Parser::make_INTEGER(constant, driver.yylloc); }
[0-9]+[ws]0[bB][01_]+   { BEGIN(driver.saveState);
                          driver.template_args = false;
                          UnparsedConstant constant{yytext, 2, 2, true};
                          return Parser::make_INTEGER(constant, driver.yylloc); }
[0-9]+[ws][0-9_]+       { BEGIN(driver.saveState);
                          driver.template_args = false;
                          UnparsedConstant constant{yytext, 0, 10, true};
                          return Parser::make_INTEGER(constant, driver.yylloc); }

"&&&"   { BEGIN(driver.saveState); driver.template_args = false; return makeToken(MASK); }
".."    { BEGIN(driver.saveState); driver.template_args = false; return makeToken(RANGE); }
"<<"    { BEGIN(driver.saveState); driver.template_args = false; return makeToken(SHL); }
"&&"    { BEGIN(driver.saveState); driver.template_args = false; return makeToken(AND); }
"||"    { BEGIN(driver.saveState); driver.template_args = false; return makeToken(OR); }
"=="    { BEGIN(driver.saveState); driver.template_args = false; return makeToken(EQ); }
"!="    { BEGIN(driver.saveState); driver.template_args = false; return makeToken(NE); }
">="    { BEGIN(driver.saveState); driver.template_args = false; return makeToken(GE); }
"<="    { BEGIN(driver.saveState); driver.template_args = false; return makeToken(LE); }
"++"    { BEGIN(driver.saveState); driver.template_args = false; return makeToken(PP); }

"+"     { BEGIN(driver.saveState); driver.template_args = false; return makeToken(PLUS); }
"|+|"   { BEGIN(driver.saveState); driver.template_args = false; return makeToken(PLUS_SAT); }
"-"     { BEGIN(driver.saveState); driver.template_args = false; return makeToken(MINUS); }
"|-|"   { BEGIN(driver.saveState); driver.template_args = false; return makeToken(MINUS_SAT); }
"*"     { BEGIN(driver.saveState); driver.template_args = false; return makeToken(MUL); }
"/"     { BEGIN(driver.saveState); driver.template_args = false; return makeToken(DIV); }
"%"     { BEGIN(driver.saveState); driver.template_args = false; return makeToken(MOD); }

"|"     { BEGIN(driver.saveState); driver.template_args = false; return makeToken(BIT_OR); }
"&"     { BEGIN(driver.saveState); driver.template_args = false; return makeToken(BIT_AND); }
"^"     { BEGIN(driver.saveState); driver.template_args = false; return makeToken(BIT_XOR); }
"~"     { BEGIN(driver.saveState); driver.template_args = false; return makeToken(COMPLEMENT); }

"("     { BEGIN(driver.saveState); driver.template_args = false; return makeToken(L_PAREN); }
")"     { BEGIN(driver.saveState); driver.template_args = false; return makeToken(R_PAREN); }
"["     { BEGIN(driver.saveState); driver.template_args = false; return makeToken(L_BRACKET); }
"]"     { BEGIN(driver.saveState); driver.template_args = false; return makeToken(R_BRACKET); }
"{"     { BEGIN(driver.saveState); driver.template_args = false; return makeToken(L_BRACE); }
"}"     { BEGIN(driver.saveState); driver.template_args = false; return makeToken(R_BRACE); }
"<"     { BEGIN(driver.saveState);
          return driver.template_args ? makeToken(L_ANGLE_ARGS) : makeToken(L_ANGLE); }
">"/">" { BEGIN(driver.saveState); driver.template_args = false; return makeToken(R_ANGLE_SHIFT); }
">"     { BEGIN(driver.saveState); driver.template_args = false; return makeToken(R_ANGLE); }

"!"     { BEGIN(driver.saveState); driver.template_args = false; return makeToken(NOT); }
":"     { BEGIN(driver.saveState); driver.template_args = false; return makeToken(COLON); }
","     { BEGIN(driver.saveState); driver.template_args = false; return makeToken(COMMA); }
"?"     { BEGIN(driver.saveState); driver.template_args = false; return makeToken(QUESTION); }
"."     { BEGIN(driver.saveState); driver.template_args = false; return makeToken(DOT); }
"="     { BEGIN(driver.saveState); driver.template_args = false; return makeToken(ASSIGN); }
";"     { BEGIN(driver.saveState); driver.template_args = false; return makeToken(SEMICOLON); }
"@"     { BEGIN(driver.saveState); driver.template_args = false; return makeToken(AT); }

<*>.|\n        { return makeToken(UNEXPECTED_TOKEN); }

%%
