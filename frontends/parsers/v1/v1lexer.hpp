#ifndef FRONTENDS_V1LEXER_H_
#define FRONTENDS_V1LEXER_H_

// FlexLexer.h requires you to provide an external #include guard so that it can
// be included multiple times, each time providing a different definition for
// yyFlexLexer, to define multiple lexer base classes.
#ifndef yyFlexLexerOnce
#define yyFlexLexer v1FlexLexer
#include <FlexLexer.h>
#endif

#include "frontends/common/constantParsing.h"
#include "frontends/parsers/v1/v1parser.hpp"
#include "lib/source_file.h"

namespace V1 {

class V1ParserDriver;

class V1Lexer : public yyFlexLexer {
    typedef V1::V1Parser::symbol_type Token;

 public:
    explicit V1Lexer(std::istream& input) : yyFlexLexer(&input) { }

    /**
     * Invoked by the parser to advance to the next token in the input stream.
     *
     * Note that we could store @driver as a member on the class, but it's
     * actually useful to explicitly pass it in. In C++, you cannot overload a
     * method on return type only. Since yyFlexLexer already declares a version
     * of yylex() that takes no arguments and returns an int, and we need to
     * return a Token, we need to add an additional argument to permit the
     * overload.
     *
     * @return the token that was just read.
     */
    virtual Token yylex(V1::V1ParserDriver& driver);

 private:
    int yylex() override { return yyFlexLexer::yylex(); }
};

}  // namespace V1

#endif  /* FRONTENDS_V1LEXER_H_ */
