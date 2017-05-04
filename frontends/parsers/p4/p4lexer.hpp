#ifndef FRONTENDS_P4_LEXER_H_
#define FRONTENDS_P4_LEXER_H_

// FlexLexer.h requires you to provide an external #include guard so that it can
// be included multiple times, each time providing a different definition for
// yyFlexLexer, to define multiple lexer base classes.
#ifndef yyFlexLexerOnce
#define yyFlexLexer p4FlexLexer
#include <FlexLexer.h>
#endif

#include "frontends/parsers/p4/p4parser.hpp"
#include "lib/source_file.h"

namespace P4 {

class P4ParserDriver;

class P4Lexer : public p4FlexLexer {
    typedef P4::P4Parser::symbol_type Token;

 public:
    explicit P4Lexer(std::istream& input) : p4FlexLexer(&input) { }

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
    virtual Token yylex(P4::P4ParserDriver& driver);

 private:
    int yylex() override { return p4FlexLexer::yylex(); }
};

}  // namespace P4

#endif  /* FRONTENDS_P4_LEXER_H_ */
