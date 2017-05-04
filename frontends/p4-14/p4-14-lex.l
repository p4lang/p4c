%{
#include "lib/stringref.h"

#define YY_USER_ACTION                                                  \
    { auto tmp = Util::InputSources::instance->getCurrentPosition();                            \
      Util::InputSources::instance->appendText(yytext);                                         \
      yylloc = Util::SourceInfo(tmp, Util::InputSources::instance->getCurrentPosition()); }
#define YY_USER_INIT saveState = NORMAL

static int lineDirectiveLine;
static int saveState = -1;

// shut up warnings about unused functions and variables
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wunused-variable"

%}

%option noyywrap nounput noinput noyyget_leng
%option noyyget_debug noyyset_debug noyyget_extra noyyset_extra noyyget_in noyyset_in
%option noyyget_out noyyset_out noyyget_text noyyget_lineno noyyset_lineno

%x COMMENT
%x LINE1 LINE2 LINE3
%s NORMAL PRAGMA_LINE

%%

[ \t\r]+                ;
<PRAGMA_LINE>[\n]     { BEGIN INITIAL; saveState = NORMAL; return '\n'; }
[\n]                  { BEGIN INITIAL; }
"//".*                  ;
"/*"                  { BEGIN COMMENT; }
<COMMENT>"*/"         { BEGIN saveState; }
<COMMENT>.              ;
<COMMENT>[\n]         { if (saveState == PRAGMA_LINE) { saveState = NORMAL; return '\n'; } }

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

"@pragma"[ \t]*[A-Za-z_][A-Za-z0-9_]* {
                  yylval.str = StringRef(yytext+7).trim();
                  BEGIN((saveState = PRAGMA_LINE));
                  return PRAGMA; }
"@pragma"[ \t]* { yylval.str = "pragma";
                  BEGIN((saveState = PRAGMA_LINE));
                  return PRAGMA; }

"action"        { yylval.str = yytext; BEGIN(saveState); return ACTION; }
"actions"       { yylval.str = yytext; BEGIN(saveState); return ACTIONS; }
"action_profile" {yylval.str = yytext; BEGIN(saveState); return ACTION_PROFILE; }
"action_selector" {
                  yylval.str = yytext; BEGIN(saveState); return ACTION_SELECTOR; }
"algorithm"     { yylval.str = yytext; BEGIN(saveState); return ALGORITHM; }
"and"           { yylval.str = yytext; BEGIN(saveState); return AND; }
"apply"         { yylval.str = yytext; BEGIN(saveState); return APPLY; }
"attribute"     { yylval.str = yytext; BEGIN(saveState); return ATTRIBUTE; }
"attributes"    { yylval.str = yytext; BEGIN(saveState); return ATTRIBUTES; }
"bit"           { yylval.str = yytext; BEGIN(saveState); return BIT; }
"bool"          { yylval.str = yytext; BEGIN(saveState); return BOOL; }
"blackbox"      { yylval.str = yytext; BEGIN(saveState); return BLACKBOX; }
"blackbox_type" { yylval.str = yytext; BEGIN(saveState); return BLACKBOX_TYPE; }
"block"         { yylval.str = yytext; BEGIN(saveState); return BLOCK; }
"calculated_field" {
                  yylval.str = yytext; BEGIN(saveState); return CALCULATED_FIELD; }
"control"       { yylval.str = yytext; BEGIN(saveState); return CONTROL; }
"counter"       { yylval.str = yytext; BEGIN(saveState); return COUNTER; }
"current"       { yylval.str = yytext; BEGIN(saveState); return CURRENT; }
"default"       { yylval.str = yytext; BEGIN(saveState); return DEFAULT; }
"default_action" {yylval.str = yytext; BEGIN(saveState); return DEFAULT_ACTION; }
"direct"        { yylval.str = yytext; BEGIN(saveState); return DIRECT; }
"drop"          { yylval.str = yytext; BEGIN(saveState); return DROP; }
"dynamic_action_selection" {
                  yylval.str = yytext; BEGIN(saveState);
                  return DYNAMIC_ACTION_SELECTION; }
"else"          { yylval.str = yytext; BEGIN(saveState); return ELSE; }
"extern"        { yylval.str = yytext; BEGIN(saveState); return BLACKBOX; }
"extern_type"   { yylval.str = yytext; BEGIN(saveState); return BLACKBOX_TYPE; }
"expression"    { yylval.str = yytext; BEGIN(saveState); return EXPRESSION; }
"expression_local_variables" {
                  yylval.str = yytext; BEGIN(saveState);
                  return EXPRESSION_LOCAL_VARIABLES; }
"extract"       { yylval.str = yytext; BEGIN(saveState); return EXTRACT; }
"false"         { yylval.str = yytext; BEGIN(saveState); return FALSE; }
"field_list"    { yylval.str = yytext; BEGIN(saveState); return FIELD_LIST; }
"field_list_calculation" {
                  yylval.str = yytext; BEGIN(saveState);
                  return FIELD_LIST_CALCULATION; }
"fields"        { yylval.str = yytext; BEGIN(saveState); return FIELDS; }
"header"        { yylval.str = yytext; BEGIN(saveState); return HEADER; }
"header_type"   { yylval.str = yytext; BEGIN(saveState); return HEADER_TYPE; }
"if"            { yylval.str = yytext; BEGIN(saveState); return IF; }
"implementation" {yylval.str = yytext; BEGIN(saveState); return IMPLEMENTATION; }
"input"         { yylval.str = yytext; BEGIN(saveState); return INPUT; }
"instance_count" {yylval.str = yytext; BEGIN(saveState); return INSTANCE_COUNT; }
"in"            { yylval.str = yytext; BEGIN(saveState); return IN; }
"int"           { yylval.str = yytext; BEGIN(saveState); return INT; }
"latest"        { yylval.str = yytext; BEGIN(saveState); return LATEST; }
"layout"        { yylval.str = yytext; BEGIN(saveState); return LAYOUT; }
"length"        { yylval.str = yytext; BEGIN(saveState); return LENGTH; }
"mask"          { yylval.str = yytext; BEGIN(saveState); return MASK; }
"max_length"    { yylval.str = yytext; BEGIN(saveState); return MAX_LENGTH; }
"max_size"      { yylval.str = yytext; BEGIN(saveState); return MAX_SIZE; }
"max_width"     { yylval.str = yytext; BEGIN(saveState); return MAX_WIDTH; }
"metadata"      { yylval.str = yytext; BEGIN(saveState); return METADATA; }
"meter"         { yylval.str = yytext; BEGIN(saveState); return METER; }
"method"        { yylval.str = yytext; BEGIN(saveState); return METHOD; }
"min_size"      { yylval.str = yytext; BEGIN(saveState); return MIN_SIZE; }
"min_width"     { yylval.str = yytext; BEGIN(saveState); return MIN_WIDTH; }
"not"           { yylval.str = yytext; BEGIN(saveState); return NOT; }
"or"            { yylval.str = yytext; BEGIN(saveState); return OR; }
"optional"      { yylval.str = yytext; BEGIN(saveState); return OPTIONAL; }
"out"           { yylval.str = yytext; BEGIN(saveState); return OUT; }
"output_width"  { yylval.str = yytext; BEGIN(saveState); return OUTPUT_WIDTH; }
"parse_error"   { yylval.str = yytext; BEGIN(saveState); return PARSE_ERROR; }
"parser"        { yylval.str = yytext; BEGIN(saveState); return PARSER; }
"parser_exception" {
                  yylval.str = yytext; BEGIN(saveState); return PARSER_EXCEPTION; }
"payload"       { yylval.str = yytext; BEGIN(saveState); return PAYLOAD; }
"pre_color"     { yylval.str = yytext; BEGIN(saveState); return PRE_COLOR; }
"primitive_action" {
                  yylval.str = yytext; BEGIN(saveState); return PRIMITIVE_ACTION; }
"reads"         { yylval.str = yytext; BEGIN(saveState); return READS; }
"register"      { yylval.str = yytext; BEGIN(saveState); return REGISTER; }
"result"        { yylval.str = yytext; BEGIN(saveState); return RESULT; }
"return"        { yylval.str = yytext; BEGIN(saveState); return RETURN; }
"saturating"    { yylval.str = yytext; BEGIN(saveState); return SATURATING; }
"select"        { yylval.str = yytext; BEGIN(saveState); return SELECT; }
"selection_key" { yylval.str = yytext; BEGIN(saveState); return SELECTION_KEY; }
"selection_mode" {yylval.str = yytext; BEGIN(saveState); return SELECTION_MODE; }
"selection_type" {yylval.str = yytext; BEGIN(saveState); return SELECTION_TYPE; }
"set_metadata"  { yylval.str = yytext; BEGIN(saveState); return SET_METADATA; }
"signed"        { yylval.str = yytext; BEGIN(saveState); return SIGNED; }
"size"          { yylval.str = yytext; BEGIN(saveState); return SIZE; }
"static"        { yylval.str = yytext; BEGIN(saveState); return STATIC; }
"string"        { yylval.str = yytext; BEGIN(saveState); return STRING; }
"true"          { yylval.str = yytext; BEGIN(saveState); return TRUE; }
"table"         { yylval.str = yytext; BEGIN(saveState); return TABLE; }
"type"          { yylval.str = yytext; BEGIN(saveState); return TYPE; }
"update"        { yylval.str = yytext; BEGIN(saveState); return UPDATE; }
"valid"         { yylval.str = yytext; BEGIN(saveState); return VALID; }
"verify"        { yylval.str = yytext; BEGIN(saveState); return VERIFY; }
"width"         { yylval.str = yytext; BEGIN(saveState); return WIDTH; }
"writes"        { yylval.str = yytext; BEGIN(saveState); return WRITES; }
[A-Za-z_][A-Za-z0-9_]* {
                  yylval.str = yytext; BEGIN(saveState); return IDENTIFIER; }

0[xX][0-9a-fA-F_]+ { yylval.Constant = new IR::Constant(yylloc, Util::cvtInt(yytext+2, 16), 16);
                  BEGIN(saveState); return INTEGER; }
0[dD][0-9_]+    { yylval.Constant = new IR::Constant(yylloc, Util::cvtInt(yytext+2, 10), 10);
                  BEGIN(saveState); return INTEGER; }
0[oO][0-7_]+    { yylval.Constant = new IR::Constant(yylloc, Util::cvtInt(yytext+2, 8), 8);
                  BEGIN(saveState); return INTEGER; }
0[bB][01_]+     { yylval.Constant = new IR::Constant(yylloc, Util::cvtInt(yytext+2, 2), 2);
                  BEGIN(saveState); return INTEGER; }
[0-9]+          { yylval.Constant = new IR::Constant(yylloc, Util::cvtInt(yytext, 10), 10);
                  BEGIN(saveState); return INTEGER; }

[0-9]+[ws']0[xX][0-9a-fA-F_]+ { yylval.Constant = cvtCst(yylloc, yytext, 2, 16);
                                BEGIN(saveState); return INTEGER; }
[0-9]+[ws']0[dD][0-9_]+ { yylval.Constant = cvtCst(yylloc, yytext, 2, 10);
                          BEGIN(saveState); return INTEGER; }
[0-9]+[ws']0[oO][0-7_]+ { yylval.Constant = cvtCst(yylloc, yytext, 2, 8);
                          BEGIN(saveState); return INTEGER; }
[0-9]+[ws']0[bB][01_]+  { yylval.Constant = cvtCst(yylloc, yytext, 2, 2);
                          BEGIN(saveState); return INTEGER; }
[0-9]+[ws'][0-9]+       { yylval.Constant = cvtCst(yylloc, yytext, 0, 10);
                          BEGIN(saveState); return INTEGER; }

<PRAGMA_LINE>[^ \t\r\n,][^ \t\r\n,]* { yylval.str = yytext; return STRING_LITERAL; }

"<<"            { BEGIN(NORMAL); return SHL; }
">>"            { BEGIN(NORMAL); return SHR; }
"&&"            { BEGIN(NORMAL); return AND; }
"||"            { BEGIN(NORMAL); return OR; }
"=="            { BEGIN(NORMAL); return EQ; }
"!="            { BEGIN(NORMAL); return NE; }
">="            { BEGIN(NORMAL); return GE; }
"<="            { BEGIN(NORMAL); return LE; }

.               { BEGIN(saveState); return *yytext; }

%%
