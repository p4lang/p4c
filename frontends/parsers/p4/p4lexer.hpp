#ifndef FRONTENDS_P4_LEXER_H_
#define FRONTENDS_P4_LEXER_H_

// FlexLexer.h requires you to provide an external #include guard so that it can
// be included multiple times, each time providing a different definition for
// yyFlexLexer, to define multiple lexer base classes.

#define yyFlexLexer p4FlexLexer
#include <FlexLexer.h>
#undef yyFlexLexer

#include "frontends/parsers/p4/p4lexer_internal.hpp"


#endif  /* FRONTENDS_P4_LEXER_H_ */
