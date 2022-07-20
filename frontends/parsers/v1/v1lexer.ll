%{
#include "frontends/common/constantParsing.h"
#include "frontends/parsers/parserDriver.h"
#include "frontends/parsers/v1/v1lexer.hpp"
#include "frontends/parsers/v1/v1parser.hpp"
#include "lib/stringref.h"

using Parser = V1::V1Parser;

#undef  YY_DECL
#define YY_DECL Parser::symbol_type V1::V1Lexer::yylex(V1::V1ParserDriver& driver)

#define YY_USER_ACTION driver.onReadToken(yytext);
#define YY_USER_INIT driver.saveState = NORMAL
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
%option yyclass="V1::V1Lexer"
%option prefix="v1"
%option nodefault noyywrap nounput noinput noyyget_leng
%option noyyget_debug noyyset_debug noyyget_extra noyyset_extra noyyget_in noyyset_in
%option noyyget_out noyyset_out noyyget_text noyyget_lineno noyyset_lineno

%x COMMENT
%x LINE1 LINE2 LINE3
%s NORMAL PRAGMA_LINE

%%

[ \t\r]+                ;
<PRAGMA_LINE>[\n]     { BEGIN INITIAL;
                        driver.saveState = NORMAL;
                        return Parser::make_NEWLINE(driver.yylloc); }
[\n]                  { BEGIN INITIAL; }
"//".*                  ;
"/*"                  { BEGIN COMMENT; }
<COMMENT>"*/"         { BEGIN driver.saveState; }
<COMMENT>.              ;
<COMMENT>[\n]         { if (driver.saveState == PRAGMA_LINE) {
                            driver.saveState = NORMAL;
                            return Parser::make_NEWLINE(driver.yylloc);
                        } }

<INITIAL>"#line"      { BEGIN(LINE1); }
<INITIAL>"# "         { BEGIN(LINE1); }
<INITIAL>[ \t]*"#"    { BEGIN(LINE3); }
<LINE1>[0-9]+         { BEGIN(LINE2); driver.onReadLineNumber(yytext); }
<LINE2>\"[^"]*        { BEGIN(LINE3); driver.onReadFileName(yytext+1); }
<LINE1,LINE2>[ \t]      ;
<LINE1,LINE2>.        { BEGIN(LINE3); }
<LINE3>.                ;
<LINE1,LINE2,LINE3>\n { BEGIN(INITIAL); }
<LINE1,LINE2,LINE3,COMMENT,NORMAL><<EOF>> { BEGIN(INITIAL); }

"@pragma"[ \t]*[A-Za-z_][A-Za-z0-9_]* {
                  BEGIN((driver.saveState = PRAGMA_LINE));
                  return Parser::make_PRAGMA(StringRef(yytext+7).trim(), driver.yylloc); }
"@pragma"[ \t]* { BEGIN((driver.saveState = PRAGMA_LINE));
                  return Parser::make_PRAGMA("pragma", driver.yylloc); }

"action"        { BEGIN(driver.saveState);
                  return Parser::make_ACTION(cstring(yytext), driver.yylloc); }
"actions"       { BEGIN(driver.saveState);
                  return Parser::make_ACTIONS(cstring(yytext), driver.yylloc); }
"action_profile" {BEGIN(driver.saveState);
                  return Parser::make_ACTION_PROFILE(cstring(yytext), driver.yylloc); }
"action_selector" { BEGIN(driver.saveState);
                    return Parser::make_ACTION_SELECTOR(cstring(yytext), driver.yylloc); }
"algorithm"     { BEGIN(driver.saveState);
                  return Parser::make_ALGORITHM(cstring(yytext), driver.yylloc); }
"and"           { BEGIN(driver.saveState);
                  return Parser::make_AND(driver.yylloc); }
"apply"         { BEGIN(driver.saveState);
                  return Parser::make_APPLY(cstring(yytext), driver.yylloc); }
"attribute"     { BEGIN(driver.saveState);
                  return Parser::make_ATTRIBUTE(cstring(yytext), driver.yylloc); }
"attributes"    { BEGIN(driver.saveState);
                  return Parser::make_ATTRIBUTES(cstring(yytext), driver.yylloc); }
"bit"           { BEGIN(driver.saveState);
                  return Parser::make_BIT(cstring(yytext), driver.yylloc); }
"bool"          { BEGIN(driver.saveState);
                  return Parser::make_BOOL(cstring(yytext), driver.yylloc); }
"blackbox"      { BEGIN(driver.saveState);
                  return Parser::make_BLACKBOX(cstring(yytext), driver.yylloc); }
"blackbox_type" { BEGIN(driver.saveState);
                  return Parser::make_BLACKBOX_TYPE(cstring(yytext), driver.yylloc); }
"block"         { BEGIN(driver.saveState);
                  return Parser::make_BLOCK(cstring(yytext), driver.yylloc); }
"calculated_field" { BEGIN(driver.saveState);
                     return Parser::make_CALCULATED_FIELD(cstring(yytext), driver.yylloc); }
"control"       { BEGIN(driver.saveState);
                  return Parser::make_CONTROL(cstring(yytext), driver.yylloc); }
"counter"       { BEGIN(driver.saveState);
                  return Parser::make_COUNTER(cstring(yytext), driver.yylloc); }
"current"       { BEGIN(driver.saveState);
                  return Parser::make_CURRENT(cstring(yytext), driver.yylloc); }
"default"       { BEGIN(driver.saveState);
                  return Parser::make_DEFAULT(cstring(yytext), driver.yylloc); }
"default_action" {BEGIN(driver.saveState);
                  return Parser::make_DEFAULT_ACTION(cstring(yytext), driver.yylloc); }
"const"         {BEGIN(driver.saveState);
                  return Parser::make_CONST(cstring(yytext), driver.yylloc); }
"direct"        { BEGIN(driver.saveState);
                  return Parser::make_DIRECT(cstring(yytext), driver.yylloc); }
"drop"          { BEGIN(driver.saveState);
                  return Parser::make_DROP(cstring(yytext), driver.yylloc); }
"dynamic_action_selection" {
                  BEGIN(driver.saveState);
                  return Parser::make_DYNAMIC_ACTION_SELECTION(cstring(yytext), driver.yylloc); }
"else"          { BEGIN(driver.saveState);
                  return Parser::make_ELSE(cstring(yytext), driver.yylloc); }
"extern"        { BEGIN(driver.saveState);
                  return Parser::make_BLACKBOX(cstring(yytext), driver.yylloc); }
"extern_type"   { BEGIN(driver.saveState);
                  return Parser::make_BLACKBOX_TYPE(cstring(yytext), driver.yylloc); }
"expression"    { BEGIN(driver.saveState);
                  return Parser::make_EXPRESSION(cstring(yytext), driver.yylloc); }
"expression_local_variables" {
                  BEGIN(driver.saveState);
                  return Parser::make_EXPRESSION_LOCAL_VARIABLES(cstring(yytext), driver.yylloc); }
"extract"       { BEGIN(driver.saveState);
                  return Parser::make_EXTRACT(cstring(yytext), driver.yylloc); }
"false"         { BEGIN(driver.saveState);
                  return Parser::make_FALSE(cstring(yytext), driver.yylloc); }
"field_list"    { BEGIN(driver.saveState);
                  return Parser::make_FIELD_LIST(cstring(yytext), driver.yylloc); }
"field_list_calculation" {
                  BEGIN(driver.saveState);
                  return Parser::make_FIELD_LIST_CALCULATION(cstring(yytext), driver.yylloc); }
"fields"        { BEGIN(driver.saveState);
                  return Parser::make_FIELDS(cstring(yytext), driver.yylloc); }
"header"        { BEGIN(driver.saveState);
                  return Parser::make_HEADER(cstring(yytext), driver.yylloc); }
"header_type"   { BEGIN(driver.saveState);
                  return Parser::make_HEADER_TYPE(cstring(yytext), driver.yylloc); }
"if"            { BEGIN(driver.saveState);
                  return Parser::make_IF(cstring(yytext), driver.yylloc); }
"implementation" {BEGIN(driver.saveState);
                  return Parser::make_IMPLEMENTATION(cstring(yytext), driver.yylloc); }
"input"         { BEGIN(driver.saveState);
                  return Parser::make_INPUT(cstring(yytext), driver.yylloc); }
"instance_count" {BEGIN(driver.saveState);
                  return Parser::make_INSTANCE_COUNT(cstring(yytext), driver.yylloc); }
"in"            { BEGIN(driver.saveState);
                  return Parser::make_IN(cstring(yytext), driver.yylloc); }
"int"           { BEGIN(driver.saveState);
                  return Parser::make_INT(cstring(yytext), driver.yylloc); }
"latest"        { BEGIN(driver.saveState);
                  return Parser::make_LATEST(cstring(yytext), driver.yylloc); }
"layout"        { BEGIN(driver.saveState);
                  return Parser::make_LAYOUT(cstring(yytext), driver.yylloc); }
"length"        { BEGIN(driver.saveState);
                  return Parser::make_LENGTH(cstring(yytext), driver.yylloc); }
"mask"          { BEGIN(driver.saveState);
                  return Parser::make_MASK(cstring(yytext), driver.yylloc); }
"max_length"    { BEGIN(driver.saveState);
                  return Parser::make_MAX_LENGTH(cstring(yytext), driver.yylloc); }
"max_size"      { BEGIN(driver.saveState);
                  return Parser::make_MAX_SIZE(cstring(yytext), driver.yylloc); }
"max_width"     { BEGIN(driver.saveState);
                  return Parser::make_MAX_WIDTH(cstring(yytext), driver.yylloc); }
"metadata"      { BEGIN(driver.saveState);
                  return Parser::make_METADATA(cstring(yytext), driver.yylloc); }
"meter"         { BEGIN(driver.saveState);
                  return Parser::make_METER(cstring(yytext), driver.yylloc); }
"method"        { BEGIN(driver.saveState);
                  return Parser::make_METHOD(cstring(yytext), driver.yylloc); }
"min_size"      { BEGIN(driver.saveState);
                  return Parser::make_MIN_SIZE(cstring(yytext), driver.yylloc); }
"min_width"     { BEGIN(driver.saveState);
                  return Parser::make_MIN_WIDTH(cstring(yytext), driver.yylloc); }
"not"           { BEGIN(driver.saveState);
                  return Parser::make_NOT(driver.yylloc); }
"or"            { BEGIN(driver.saveState);
                  return Parser::make_OR(driver.yylloc); }
"optional"      { BEGIN(driver.saveState);
                  return Parser::make_OPTIONAL(cstring(yytext), driver.yylloc); }
"out"           { BEGIN(driver.saveState);
                  return Parser::make_OUT(cstring(yytext), driver.yylloc); }
"output_width"  { BEGIN(driver.saveState);
                  return Parser::make_OUTPUT_WIDTH(cstring(yytext), driver.yylloc); }
"parse_error"   { BEGIN(driver.saveState);
                  return Parser::make_PARSE_ERROR(cstring(yytext), driver.yylloc); }
"parser"        { BEGIN(driver.saveState);
                  return Parser::make_PARSER(cstring(yytext), driver.yylloc); }
"parser_value_set" { BEGIN(driver.saveState);
                     return Parser::make_PARSER_VALUE_SET(cstring(yytext), driver.yylloc); }
"parser_exception" { BEGIN(driver.saveState);
                     return Parser::make_PARSER_EXCEPTION(cstring(yytext), driver.yylloc); }
"payload"       { BEGIN(driver.saveState);
                  return Parser::make_PAYLOAD(cstring(yytext), driver.yylloc); }
"pre_color"     { BEGIN(driver.saveState);
                  return Parser::make_PRE_COLOR(cstring(yytext), driver.yylloc); }
"primitive_action" { BEGIN(driver.saveState);
                     return Parser::make_PRIMITIVE_ACTION(cstring(yytext), driver.yylloc); }
"reads"         { BEGIN(driver.saveState);
                  return Parser::make_READS(cstring(yytext), driver.yylloc); }
"register"      { BEGIN(driver.saveState);
                  return Parser::make_REGISTER(cstring(yytext), driver.yylloc); }
"result"        { BEGIN(driver.saveState);
                  return Parser::make_RESULT(cstring(yytext), driver.yylloc); }
"return"        { BEGIN(driver.saveState);
                  return Parser::make_RETURN(cstring(yytext), driver.yylloc); }
"saturating"    { BEGIN(driver.saveState);
                  return Parser::make_SATURATING(cstring(yytext), driver.yylloc); }
"select"        { BEGIN(driver.saveState);
                  return Parser::make_SELECT(cstring(yytext), driver.yylloc); }
"selection_key" { BEGIN(driver.saveState);
                  return Parser::make_SELECTION_KEY(cstring(yytext), driver.yylloc); }
"selection_mode" {BEGIN(driver.saveState);
                  return Parser::make_SELECTION_MODE(cstring(yytext), driver.yylloc); }
"selection_type" {BEGIN(driver.saveState);
                  return Parser::make_SELECTION_TYPE(cstring(yytext), driver.yylloc); }
"set_metadata"  { BEGIN(driver.saveState);
                  return Parser::make_SET_METADATA(cstring(yytext), driver.yylloc); }
"signed"        { BEGIN(driver.saveState);
                  return Parser::make_SIGNED(cstring(yytext), driver.yylloc); }
"size"          { BEGIN(driver.saveState);
                  return Parser::make_SIZE(cstring(yytext), driver.yylloc); }
"static"        { BEGIN(driver.saveState);
                  return Parser::make_STATIC(cstring(yytext), driver.yylloc); }
"string"        { BEGIN(driver.saveState);
                  return Parser::make_STRING(cstring(yytext), driver.yylloc); }
"true"          { BEGIN(driver.saveState);
                  return Parser::make_TRUE(cstring(yytext), driver.yylloc); }
"table"         { BEGIN(driver.saveState);
                  return Parser::make_TABLE(cstring(yytext), driver.yylloc); }
"type"          { BEGIN(driver.saveState);
                  return Parser::make_TYPE(cstring(yytext), driver.yylloc); }
"update"        { BEGIN(driver.saveState);
                  return Parser::make_UPDATE(cstring(yytext), driver.yylloc); }
"valid"         { BEGIN(driver.saveState);
                  return Parser::make_VALID(cstring(yytext), driver.yylloc); }
"verify"        { BEGIN(driver.saveState);
                  return Parser::make_VERIFY(cstring(yytext), driver.yylloc); }
"width"         { BEGIN(driver.saveState);
                  return Parser::make_WIDTH(cstring(yytext), driver.yylloc); }
"writes"        { BEGIN(driver.saveState);
                  return Parser::make_WRITES(cstring(yytext), driver.yylloc); }
[A-Za-z_][A-Za-z0-9_]* {
                  BEGIN(driver.saveState);
                  cstring name = yytext;
                  driver.onReadIdentifier(name);
                  return Parser::make_IDENTIFIER(name, driver.yylloc);
}

0[xX][0-9a-fA-F_]+ { BEGIN(driver.saveState);
                     UnparsedConstant constant{yytext, 2, 16, false};
                     return Parser::make_INTEGER(constant, driver.yylloc); }
0[dD][0-9_]+    { BEGIN(driver.saveState);
                  UnparsedConstant constant{yytext, 2, 10, false};
                  return Parser::make_INTEGER(constant, driver.yylloc); }
0[oO][0-7_]+    { BEGIN(driver.saveState);
                  UnparsedConstant constant{yytext, 2, 8, false};
                  return Parser::make_INTEGER(constant, driver.yylloc); }
0[bB][01_]+     { BEGIN(driver.saveState);
                  UnparsedConstant constant{yytext, 2, 2, false};
                  return Parser::make_INTEGER(constant, driver.yylloc); }
[0-9]+          { BEGIN(driver.saveState);
                  UnparsedConstant constant{yytext, 0, 10, false};
                  return Parser::make_INTEGER(constant, driver.yylloc); }

[0-9]+[ws']0[xX][0-9a-fA-F_]+ { BEGIN(driver.saveState);
                                UnparsedConstant constant{yytext, 2, 16, true};
                                return Parser::make_INTEGER(constant, driver.yylloc); }
[0-9]+[ws']0[dD][0-9_]+ { BEGIN(driver.saveState);
                          UnparsedConstant constant{yytext, 2, 10, true};
                          return Parser::make_INTEGER(constant, driver.yylloc); }
[0-9]+[ws']0[oO][0-7_]+ { BEGIN(driver.saveState);
                          UnparsedConstant constant{yytext, 2, 8, true};
                          return Parser::make_INTEGER(constant, driver.yylloc); }
[0-9]+[ws']0[bB][01_]+  { BEGIN(driver.saveState);
                          UnparsedConstant constant{yytext, 2, 2, true};
                          return Parser::make_INTEGER(constant, driver.yylloc); }
[0-9]+[ws'][0-9]+       { BEGIN(driver.saveState);
                          UnparsedConstant constant{yytext, 0, 10, true};
                          return Parser::make_INTEGER(constant, driver.yylloc); }

[0-9]+[ \t\r]*['][ \t\r]*0[xX][0-9a-fA-F_]+ {
                BEGIN(driver.saveState);
                UnparsedConstant constant{yytext, 2, 16, true};
                return Parser::make_INTEGER(constant, driver.yylloc); }
[0-9]+[ \t\r]*['][ \t\r]*0[dD][0-9_]+ {
                BEGIN(driver.saveState);
                UnparsedConstant constant{yytext, 2, 10, true};
                return Parser::make_INTEGER(constant, driver.yylloc); }
[0-9]+[ \t\r]*['][ \t\r]*0[oO][0-7_]+ {
                BEGIN(driver.saveState);
                UnparsedConstant constant{yytext, 2, 8, true};
                return Parser::make_INTEGER(constant, driver.yylloc); }
[0-9]+[ \t\r]*['][ \t\r]*0[bB][01_]+  {
                BEGIN(driver.saveState);
                UnparsedConstant constant{yytext, 2, 2, true};
                return Parser::make_INTEGER(constant, driver.yylloc); }
[0-9]+[ \t\r]*['][ \t\r]*[0-9]+       {
                BEGIN(driver.saveState);
                UnparsedConstant constant{yytext, 0, 10, true};
                return Parser::make_INTEGER(constant, driver.yylloc); }

<PRAGMA_LINE>[^ \t\r\n,][^ \t\r\n,]* { return Parser::make_STRING_LITERAL(yytext, driver.yylloc); }

"<<"            { BEGIN(driver.saveState); return Parser::make_SHL(driver.yylloc); }
">>"            { BEGIN(driver.saveState); return Parser::make_SHR(driver.yylloc); }
"&&"            { BEGIN(driver.saveState); return Parser::make_AND(driver.yylloc); }
"||"            { BEGIN(driver.saveState); return Parser::make_OR(driver.yylloc); }
"=="            { BEGIN(driver.saveState); return Parser::make_EQ(driver.yylloc); }
"!="            { BEGIN(driver.saveState); return Parser::make_NE(driver.yylloc); }
">="            { BEGIN(driver.saveState); return Parser::make_GE(driver.yylloc); }
"<="            { BEGIN(driver.saveState); return Parser::make_LE(driver.yylloc); }

"+"            { BEGIN(driver.saveState); return Parser::make_PLUS(driver.yylloc); }
"-"            { BEGIN(driver.saveState); return Parser::make_MINUS(driver.yylloc); }
"*"            { BEGIN(driver.saveState); return Parser::make_MUL(driver.yylloc); }
"/"            { BEGIN(driver.saveState); return Parser::make_DIV(driver.yylloc); }
"%"            { BEGIN(driver.saveState); return Parser::make_MOD(driver.yylloc); }

"|"            { BEGIN(driver.saveState); return Parser::make_BIT_OR(driver.yylloc); }
"&"            { BEGIN(driver.saveState); return Parser::make_BIT_AND(driver.yylloc); }
"^"            { BEGIN(driver.saveState); return Parser::make_BIT_XOR(driver.yylloc); }
"~"            { BEGIN(driver.saveState); return Parser::make_COMPLEMENT(driver.yylloc); }

"("            { BEGIN(driver.saveState); return Parser::make_L_PAREN(driver.yylloc); }
")"            { BEGIN(driver.saveState); return Parser::make_R_PAREN(driver.yylloc); }
"["            { BEGIN(driver.saveState); return Parser::make_L_BRACKET(driver.yylloc); }
"]"            { BEGIN(driver.saveState); return Parser::make_R_BRACKET(driver.yylloc); }
"{"            { BEGIN(driver.saveState); return Parser::make_L_BRACE(driver.yylloc); }
"}"            { BEGIN(driver.saveState); return Parser::make_R_BRACE(driver.yylloc); }
"<"            { BEGIN(driver.saveState); return Parser::make_L_ANGLE(driver.yylloc); }
">"            { BEGIN(driver.saveState); return Parser::make_R_ANGLE(driver.yylloc); }

"!"            { BEGIN(driver.saveState); return Parser::make_NOT(driver.yylloc); }
":"            { BEGIN(driver.saveState); return Parser::make_COLON(driver.yylloc); }
","            { BEGIN(driver.saveState); return Parser::make_COMMA(driver.yylloc); }
"."            { BEGIN(driver.saveState); return Parser::make_DOT(driver.yylloc); }
"="            { BEGIN(driver.saveState); return Parser::make_ASSIGN(driver.yylloc); }
";"            { BEGIN(driver.saveState); return Parser::make_SEMICOLON(driver.yylloc); }

.              { return Parser::make_UNEXPECTED_TOKEN(driver.yylloc); }

%%
