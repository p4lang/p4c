#ifndef FRONTENDS_P4_ABSTRACTP4LEXER_H_
#define FRONTENDS_P4_ABSTRACTP4LEXER_H_

#include "frontends/parsers/p4/p4parser.hpp"

namespace P4 {

class AbstractP4Lexer {
 protected:
    typedef P4::P4Parser::symbol_type Token;

 public:
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
    virtual Token yylex(P4::P4ParserDriver& driver) = 0;
};

}  // namespace P4

#endif  /* FRONTENDS_P4_ABSTRACTP4LEXER_H_ */
