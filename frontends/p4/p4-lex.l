%{
#define YY_USER_ACTION                                                                          \
    { auto tmp = Util::InputSources::instance->getCurrentPosition();                            \
      Util::InputSources::instance->appendText(yytext);                                         \
      yylloc = Util::SourceInfo(tmp, Util::InputSources::instance->getCurrentPosition()); }

// shut up warnings about unused functions and variables
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wunused-variable"

// line indicated by #line directive
static int lineDirectiveLine;
// file indicated by #line directive
static cstring lineDirectiveFile;

void parseLineNumber(const char* text)
{
    char* last;
    ::lineDirectiveLine = strtol(text, &last, 10);
    if (errno != 0 ||
        // we have not parsed the complete string
        strlen(last) != 0)
        ErrorReporter::instance.parser_error("Error parsing line number %s", text);
}

std::string stringLiteral;

%}

%option noyywrap nounput noinput noyyget_leng
%option noyyget_debug noyyset_debug noyyget_extra noyyset_extra noyyget_in noyyset_in
%option noyyget_out noyyset_out noyyget_text noyyget_lineno noyyset_lineno

%x COMMENT STRING
%x LINE1 LINE2 LINE3
%s NORMAL

%%

[ \t\r]+        ;
[\n]            { BEGIN INITIAL; }
"//".*                  ;
"/*"            { BEGIN COMMENT; }
<COMMENT>"*/"   { BEGIN NORMAL; }
<COMMENT>.              ;
<COMMENT>[\n]           ;

<INITIAL>"#line"      { BEGIN(LINE1); }
<INITIAL>"# "         { BEGIN(LINE1); }
<INITIAL>[ \t]*"#"    { BEGIN(LINE3); }
<LINE1>[0-9]+         { parseLineNumber(yytext);
                        BEGIN(LINE2); }
<LINE2>\"[^\"]*        { lineDirectiveFile = cstring(yytext+1);
                        Util::InputSources::instance->mapLine(lineDirectiveFile, lineDirectiveLine);
                        BEGIN(LINE3); }
<LINE1,LINE2>[ \t]      ;
<LINE1,LINE2>.        { BEGIN(LINE3); }
<LINE3>.                ;
<LINE1,LINE2,LINE3>\n { BEGIN(INITIAL); }
<LINE1,LINE2,LINE3,COMMENT,NORMAL><<EOF>> { BEGIN(INITIAL); }

\"              { BEGIN(STRING); stringLiteral = ""; }
<STRING>\\\"    { stringLiteral += yytext; }
<STRING>\\\\    { stringLiteral += yytext; }
<STRING>\"      { BEGIN(INITIAL);
                  yylval.str = cstring(stringLiteral);
                  return(STRING_LITERAL); }
<STRING>.       { stringLiteral += yytext; }
<STRING>\n      { stringLiteral += yytext; }

"abstract"      { BEGIN(NORMAL); return ABSTRACT; }
"action"        { BEGIN(NORMAL); return ACTION; }
"actions"       { BEGIN(NORMAL); return ACTIONS; }
"apply"         { BEGIN(NORMAL); return APPLY; }
"bool"          { BEGIN(NORMAL); return BOOL; }
"bit"           { BEGIN(NORMAL); return BIT; }
"const"         { BEGIN(NORMAL); return CONST; }
"control"       { BEGIN(NORMAL); return CONTROL; }
"default"       { BEGIN(NORMAL); return DEFAULT; }
"else"          { BEGIN(NORMAL); return ELSE; }
"enum"          { BEGIN(NORMAL); return ENUM; }
"error"         { BEGIN(NORMAL); return T_ERROR; }
"exit"          { BEGIN(NORMAL); return EXIT; }
"extern"        { BEGIN(NORMAL); return EXTERN; }
"false"         { BEGIN(NORMAL); return FALSE; }
"header"        { BEGIN(NORMAL); return HEADER; }
"header_union"  { BEGIN(NORMAL); return HEADER_UNION; }
"if"            { BEGIN(NORMAL); return IF; }
"in"            { BEGIN(NORMAL); return IN; }
"inout"         { BEGIN(NORMAL); return INOUT; }
"int"           { BEGIN(NORMAL); return INT; }
"key"           { BEGIN(NORMAL); return KEY; }
"match_kind"    { BEGIN(NORMAL); return MATCH_KIND; }
"out"           { BEGIN(NORMAL); return OUT; }
"parser"        { BEGIN(NORMAL); return PARSER; }
"package"       { BEGIN(NORMAL); return PACKAGE; }
"return"        { BEGIN(NORMAL); return RETURN; }
"select"        { BEGIN(NORMAL); return SELECT; }
"set"           { BEGIN(NORMAL); return SET; }
"state"         { BEGIN(NORMAL); return STATE; }
"struct"        { BEGIN(NORMAL); return STRUCT; }
"switch"        { BEGIN(NORMAL); return SWITCH; }
"table"         { BEGIN(NORMAL); return TABLE; }
"this"          { BEGIN(NORMAL); return THIS; }
"transition"    { BEGIN(NORMAL); return TRANSITION; }
"true"          { BEGIN(NORMAL); return TRUE; }
"tuple"         { BEGIN(NORMAL); return TUPLE; }
"typedef"       { BEGIN(NORMAL); return TYPEDEF; }
"varbit"        { BEGIN(NORMAL); return VARBIT; }
"void"          { BEGIN(NORMAL); return VOID; }
"_"             { BEGIN(NORMAL); return DONTCARE; }
[A-Za-z_][A-Za-z0-9_]* {
                  yylval.str = cstring(yytext);
                  BEGIN(NORMAL);
                  Util::ProgramStructure::SymbolKind kind = structure.lookupIdentifier(yylval.str);
                  switch (kind)
                  {
                  /* FIXME: if the type is a reserved keyword this doesn't work */
                  case Util::ProgramStructure::SymbolKind::Identifier:
                      return IDENTIFIER;
                  case Util::ProgramStructure::SymbolKind::Type:
                      return TYPE;
                  case Util::ProgramStructure::SymbolKind::Namespace:
                      return NAMESPACE;
                  default:
                      BUG("Unexpected symbol kind");
                  }
                }

0[xX][0-9a-fA-F_]+ { yylval.Constant = new IR::Constant(yylloc, Util::cvtInt(yytext+2, 16), 16);
                     BEGIN(NORMAL); return INTEGER; }
0[dD][0-9_]+       { yylval.Constant = new IR::Constant(yylloc, Util::cvtInt(yytext+2, 10), 10);
                     BEGIN(NORMAL); return INTEGER; }
0[oO][0-7_]+       { yylval.Constant = new IR::Constant(yylloc, Util::cvtInt(yytext+2, 8), 8);
                     BEGIN(NORMAL); return INTEGER; }
0[bB][01_]+        { yylval.Constant = new IR::Constant(yylloc, Util::cvtInt(yytext+2, 2), 2);
                     BEGIN(NORMAL); return INTEGER; }
[0-9][0-9_]*       { yylval.Constant = new IR::Constant(yylloc, Util::cvtInt(yytext, 10), 10);
                     BEGIN(NORMAL); return INTEGER; }

[0-9]+[ws]0[xX][0-9a-fA-F_]+ { yylval.Constant = cvtCst(yylloc, yytext, 2, 16);
                               BEGIN(NORMAL); return INTEGER; }
[0-9]+[ws]0[dD][0-9_]+  { yylval.Constant = cvtCst(yylloc, yytext, 2, 10);
                          BEGIN(NORMAL); return INTEGER; }
[0-9]+[ws]0[oO][0-7_]+  { yylval.Constant = cvtCst(yylloc, yytext, 2, 8);
                          BEGIN(NORMAL); return INTEGER; }
[0-9]+[ws]0[bB][01_]+   { yylval.Constant = cvtCst(yylloc, yytext, 2, 2);
                          BEGIN(NORMAL); return INTEGER; }
[0-9]+[ws][0-9_]+       { yylval.Constant = cvtCst(yylloc, yytext, 0, 10);
                          BEGIN(NORMAL); return INTEGER; }

"&&&"           { BEGIN(NORMAL); return MASK; }
".."            { BEGIN(NORMAL); return RANGE; }
"<<"            { BEGIN(NORMAL); return SHL; }
"&&"            { BEGIN(NORMAL); return AND; }
"||"            { BEGIN(NORMAL); return OR; }
"=="            { BEGIN(NORMAL); return EQ; }
"!="            { BEGIN(NORMAL); return NE; }
">="            { BEGIN(NORMAL); return GE; }
"<="            { BEGIN(NORMAL); return LE; }
"++"            { BEGIN(NORMAL); return PP; }

.               { BEGIN(NORMAL); return *yytext; }

%%
