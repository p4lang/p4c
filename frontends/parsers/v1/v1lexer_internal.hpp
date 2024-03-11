#ifndef FRONTENDS_PARSERS_V1_V1LEXER_INTERNAL_HPP_
#define FRONTENDS_PARSERS_V1_V1LEXER_INTERNAL_HPP_

#include <string_view>

#include "frontends/common/constantParsing.h"
#include "frontends/parsers/v1/v1parser.hpp"
#include "lib/source_file.h"

namespace V1 {

class V1ParserDriver;

class V1Lexer : public v1FlexLexer {
    typedef V1::V1Parser::symbol_type Token;

 public:
    explicit V1Lexer(std::istream &input) : v1FlexLexer(&input) {}

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
    virtual Token yylex(V1::V1ParserDriver &driver);

    static constexpr std::string_view trim(std::string_view in,
                                           std::string_view white = " \n\r\t\v") {
        auto left = in.find_first_not_of(white);
        if (left == std::string_view::npos) return {};

        in.remove_prefix(left);

        auto right = in.find_last_not_of(white);
        if (right == std::string_view::npos) return {};

        in.remove_suffix(in.size() - right - 1);
        return in;
    }

 private:
    int yylex() override { return v1FlexLexer::yylex(); }
};

}  // namespace V1

#endif /* FRONTENDS_PARSERS_V1_V1LEXER_INTERNAL_HPP_ */
