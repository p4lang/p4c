#ifndef FRONTENDS_V1LEXER_H_
#define FRONTENDS_V1LEXER_H_

// FlexLexer.h requires you to provide an external #include guard so that it can
// be included multiple times, each time providing a different definition for
// yyFlexLexer, to define multiple lexer base classes.

#define yyFlexLexer v1FlexLexer
#include <FlexLexer.h>
#undef yyFlexLexer

#include "frontends/parsers/v1/v1lexer_internal.hpp"

#endif  /* FRONTENDS_V1LEXER_H_ */
