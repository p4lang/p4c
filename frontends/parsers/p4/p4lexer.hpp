#ifndef FRONTENDS_P4_LEXER_H_
#define FRONTENDS_P4_LEXER_H_

// FlexLexer.h requires you to provide an external #include guard so that it can
// be included multiple times, each time providing a different definition for
// yyFlexLexer, to define multiple lexer base classes.
#ifndef yyFlexLexerOnce
#define yyFlexLexer p4FlexLexer
#include <FlexLexer.h>
#endif

#include "frontends/parsers/p4/abstractP4Lexer.hpp"
#include "frontends/parsers/p4/p4parser.hpp"
#include "lib/source_file.h"

namespace P4 {

class P4ParserDriver;

class P4Lexer : public AbstractP4Lexer, public p4FlexLexer {
 public:
    explicit P4Lexer(std::istream& input)
        : p4FlexLexer(&input), needStartToken(true) { }

    virtual Token yylex(P4::P4ParserDriver& driver) override;

 private:
    bool needStartToken;
    int yylex() override { return p4FlexLexer::yylex(); }
};

}  // namespace P4

#endif  /* FRONTENDS_P4_LEXER_H_ */
