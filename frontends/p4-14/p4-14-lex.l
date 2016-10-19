%{
#include "lib/stringref.h"

#define YY_USER_ACTION                                                  \
    { auto tmp = Util::InputSources::instance->getCurrentPosition();                            \
      Util::InputSources::instance->appendText(yytext);                                         \
      yylloc = Util::SourceInfo(tmp, Util::InputSources::instance->getCurrentPosition()); }

static int lineDirectiveLine;

// shut up warnings about unused functions and variables
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wunused-variable"

%}

%option noyywrap nounput noinput noyyget_leng
%option noyyget_debug noyyset_debug noyyget_extra noyyset_extra noyyget_in noyyset_in
%option noyyget_out noyyset_out noyyget_text noyyget_lineno noyyset_lineno

%x COMMENT
%x LINE1 LINE2 LINE3
%s NORMAL

%%

[ \t\r]+                ;
[\n]            { BEGIN INITIAL; }
"//".*                  ;
"/*"            { BEGIN COMMENT; }
<COMMENT>"*/"   { BEGIN NORMAL; }
<COMMENT>.              ;
<COMMENT>[\n]           ;

<INITIAL>"#line"      { BEGIN(LINE1); }
<INITIAL>"# "         { BEGIN(LINE1); }
<INITIAL>[ \t]*"#"    { BEGIN(LINE3); }
<LINE1>[0-9]+         { lineDirectiveLine = atoi(yytext);
                        BEGIN(LINE2); }
<LINE2>\"[^"]*        { Util::InputSources::instance->mapLine(yytext+1, lineDirectiveLine);
                        BEGIN(LINE3); }
<LINE1,LINE2>[ \t]      ;
<LINE1,LINE2>.        { BEGIN(LINE3); }
<LINE3>.                ;
<LINE1,LINE2,LINE3>\n { BEGIN(INITIAL); }
<LINE1,LINE2,LINE3,COMMENT,NORMAL><<EOF>> { BEGIN(INITIAL); }

"@pragma".*     { yylval.str = StringRef(yytext+7).trim(); return PRAGMA; }

"action"        { yylval.str = yytext; BEGIN(NORMAL); return ACTION; }
"actions"       { yylval.str = yytext; BEGIN(NORMAL); return ACTIONS; }
"action_profile" {yylval.str = yytext; BEGIN(NORMAL); return ACTION_PROFILE; }
"action_selector" {
                  yylval.str = yytext; BEGIN(NORMAL); return ACTION_SELECTOR; }
"algorithm"     { yylval.str = yytext; BEGIN(NORMAL); return ALGORITHM; }
"and"           { yylval.str = yytext; BEGIN(NORMAL); return AND; }
"apply"         { yylval.str = yytext; BEGIN(NORMAL); return APPLY; }
"attribute"     { yylval.str = yytext; BEGIN(NORMAL); return ATTRIBUTE; }
"attributes"    { yylval.str = yytext; BEGIN(NORMAL); return ATTRIBUTES; }
"bit"           { yylval.str = yytext; BEGIN(NORMAL); return BIT; }
"blackbox"      { yylval.str = yytext; BEGIN(NORMAL); return BLACKBOX; }
"blackbox_type" { yylval.str = yytext; BEGIN(NORMAL); return BLACKBOX_TYPE; }
"calculated_field" {
                  yylval.str = yytext; BEGIN(NORMAL); return CALCULATED_FIELD; }
"control"       { yylval.str = yytext; BEGIN(NORMAL); return CONTROL; }
"counter"       { yylval.str = yytext; BEGIN(NORMAL); return COUNTER; }
"current"       { yylval.str = yytext; BEGIN(NORMAL); return CURRENT; }
"default"       { yylval.str = yytext; BEGIN(NORMAL); return DEFAULT; }
"default_action" {yylval.str = yytext; BEGIN(NORMAL); return DEFAULT_ACTION; }
"direct"        { yylval.str = yytext; BEGIN(NORMAL); return DIRECT; }
"drop"          { yylval.str = yytext; BEGIN(NORMAL); return DROP; }
"dynamic_action_selection" {
                  yylval.str = yytext; BEGIN(NORMAL);
                  return DYNAMIC_ACTION_SELECTION; }
"else"          { yylval.str = yytext; BEGIN(NORMAL); return ELSE; }
"extract"       { yylval.str = yytext; BEGIN(NORMAL); return EXTRACT; }
"false"         { yylval.str = yytext; BEGIN(NORMAL); return FALSE; }
"field_list"    { yylval.str = yytext; BEGIN(NORMAL); return FIELD_LIST; }
"field_list_calculation" {
                  yylval.str = yytext; BEGIN(NORMAL);
                  return FIELD_LIST_CALCULATION; }
"fields"        { yylval.str = yytext; BEGIN(NORMAL); return FIELDS; }
"header"        { yylval.str = yytext; BEGIN(NORMAL); return HEADER; }
"header_type"   { yylval.str = yytext; BEGIN(NORMAL); return HEADER_TYPE; }
"if"            { yylval.str = yytext; BEGIN(NORMAL); return IF; }
"implementation" {yylval.str = yytext; BEGIN(NORMAL); return IMPLEMENTATION; }
"input"         { yylval.str = yytext; BEGIN(NORMAL); return INPUT; }
"instance_count" {yylval.str = yytext; BEGIN(NORMAL); return INSTANCE_COUNT; }
"int"           { yylval.str = yytext; BEGIN(NORMAL); return INT; }
"latest"        { yylval.str = yytext; BEGIN(NORMAL); return LATEST; }
"layout"        { yylval.str = yytext; BEGIN(NORMAL); return LAYOUT; }
"length"        { yylval.str = yytext; BEGIN(NORMAL); return LENGTH; }
"mask"          { yylval.str = yytext; BEGIN(NORMAL); return MASK; }
"max_length"    { yylval.str = yytext; BEGIN(NORMAL); return MAX_LENGTH; }
"max_size"      { yylval.str = yytext; BEGIN(NORMAL); return MAX_SIZE; }
"max_width"     { yylval.str = yytext; BEGIN(NORMAL); return MAX_WIDTH; }
"metadata"      { yylval.str = yytext; BEGIN(NORMAL); return METADATA; }
"meter"         { yylval.str = yytext; BEGIN(NORMAL); return METER; }
"method"        { yylval.str = yytext; BEGIN(NORMAL); return METHOD; }
"min_size"      { yylval.str = yytext; BEGIN(NORMAL); return MIN_SIZE; }
"min_width"     { yylval.str = yytext; BEGIN(NORMAL); return MIN_WIDTH; }
"not"           { yylval.str = yytext; BEGIN(NORMAL); return NOT; }
"or"            { yylval.str = yytext; BEGIN(NORMAL); return OR; }
"output_width"  { yylval.str = yytext; BEGIN(NORMAL); return OUTPUT_WIDTH; }
"parse_error"   { yylval.str = yytext; BEGIN(NORMAL); return PARSE_ERROR; }
"parser"        { yylval.str = yytext; BEGIN(NORMAL); return PARSER; }
"parser_exception" {
                  yylval.str = yytext; BEGIN(NORMAL); return PARSER_EXCEPTION; }
"payload"       { yylval.str = yytext; BEGIN(NORMAL); return PAYLOAD; }
"pre_color"     { yylval.str = yytext; BEGIN(NORMAL); return PRE_COLOR; }
"primitive_action" {
                  yylval.str = yytext; BEGIN(NORMAL); return PRIMITIVE_ACTION; }
"reads"         { yylval.str = yytext; BEGIN(NORMAL); return READS; }
"register"      { yylval.str = yytext; BEGIN(NORMAL); return REGISTER; }
"result"        { yylval.str = yytext; BEGIN(NORMAL); return RESULT; }
"return"        { yylval.str = yytext; BEGIN(NORMAL); return RETURN; }
"saturating"    { yylval.str = yytext; BEGIN(NORMAL); return SATURATING; }
"select"        { yylval.str = yytext; BEGIN(NORMAL); return SELECT; }
"selection_key" { yylval.str = yytext; BEGIN(NORMAL); return SELECTION_KEY; }
"selection_mode" {yylval.str = yytext; BEGIN(NORMAL); return SELECTION_MODE; }
"selection_type" {yylval.str = yytext; BEGIN(NORMAL); return SELECTION_TYPE; }
"set_metadata"  { yylval.str = yytext; BEGIN(NORMAL); return SET_METADATA; }
"signed"        { yylval.str = yytext; BEGIN(NORMAL); return SIGNED; }
"size"          { yylval.str = yytext; BEGIN(NORMAL); return SIZE; }
"static"        { yylval.str = yytext; BEGIN(NORMAL); return STATIC; }
"true"          { yylval.str = yytext; BEGIN(NORMAL); return TRUE; }
"table"         { yylval.str = yytext; BEGIN(NORMAL); return TABLE; }
"type"          { yylval.str = yytext; BEGIN(NORMAL); return TYPE; }
"update"        { yylval.str = yytext; BEGIN(NORMAL); return UPDATE; }
"valid"         { yylval.str = yytext; BEGIN(NORMAL); return VALID; }
"verify"        { yylval.str = yytext; BEGIN(NORMAL); return VERIFY; }
"width"         { yylval.str = yytext; BEGIN(NORMAL); return WIDTH; }
"writes"        { yylval.str = yytext; BEGIN(NORMAL); return WRITES; }
[A-Za-z_][A-Za-z0-9_]* {
                  yylval.str = yytext; BEGIN(NORMAL); return IDENTIFIER; }

0[xX][0-9a-fA-F_]+ { yylval.Constant = new IR::Constant(yylloc, Util::cvtInt(yytext+2, 16), 16);
                  BEGIN(NORMAL); return INTEGER; }
0[dD][0-9_]+    { yylval.Constant = new IR::Constant(yylloc, Util::cvtInt(yytext+2, 10), 10);
                  BEGIN(NORMAL); return INTEGER; }
0[oO][0-7_]+    { yylval.Constant = new IR::Constant(yylloc, Util::cvtInt(yytext+2, 8), 8);
                  BEGIN(NORMAL); return INTEGER; }
0[bB][01_]+     { yylval.Constant = new IR::Constant(yylloc, Util::cvtInt(yytext+2, 2), 2);
                  BEGIN(NORMAL); return INTEGER; }
[0-9]+          { yylval.Constant = new IR::Constant(yylloc, Util::cvtInt(yytext, 10), 10);
                  BEGIN(NORMAL); return INTEGER; }

[0-9]+[ws']0[xX][0-9a-fA-F_]+ { yylval.Constant = cvtCst(yylloc, yytext, 2, 16);
                                BEGIN(NORMAL); return INTEGER; }
[0-9]+[ws']0[dD][0-9_]+ { yylval.Constant = cvtCst(yylloc, yytext, 2, 10);
                          BEGIN(NORMAL); return INTEGER; }
[0-9]+[ws']0[oO][0-7_]+ { yylval.Constant = cvtCst(yylloc, yytext, 2, 8);
                          BEGIN(NORMAL); return INTEGER; }
[0-9]+[ws']0[bB][01_]+  { yylval.Constant = cvtCst(yylloc, yytext, 2, 2);
                          BEGIN(NORMAL); return INTEGER; }
[0-9]+[ws'][0-9]+       { yylval.Constant = cvtCst(yylloc, yytext, 0, 10);
                          BEGIN(NORMAL); return INTEGER; }

"<<"            { BEGIN(NORMAL); return SHL; }
">>"            { BEGIN(NORMAL); return SHR; }
"&&"            { BEGIN(NORMAL); return AND; }
"||"            { BEGIN(NORMAL); return OR; }
"=="            { BEGIN(NORMAL); return EQ; }
"!="            { BEGIN(NORMAL); return NE; }
">="            { BEGIN(NORMAL); return GE; }
"<="            { BEGIN(NORMAL); return LE; }

.               { BEGIN(NORMAL); return *yytext; }

%%
