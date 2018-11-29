#include "frontends/parsers/p4/p4AnnotationLexer.hpp"
#include "frontends/common/constantParsing.h"

namespace P4 {

P4AnnotationLexer::Token P4AnnotationLexer::yylex(P4::P4ParserDriver& driver) {
    // Driver not used here. Suppress compiler warning.
    (void) driver;

    if (needStart) {
        needStart = false;
        auto& srcInfo = body.srcInfo;
        return P4Parser::symbol_type((P4Parser::token_type) type, srcInfo);
    }

    if (it == body.end()) {
        auto& srcInfo = body.srcInfo;
        return P4Parser::make_END(srcInfo);
    }

    auto cur = *(it++);
    switch (cur->token_type) {
    case P4Parser::token_type::TOK_IDENTIFIER:
    case P4Parser::token_type::TOK_TYPE_IDENTIFIER:
        {
            auto& text = cur->text;
            auto& srcInfo = cur->srcInfo;
            return Token((P4Parser::token_type) cur->token_type, text,
                         srcInfo);
        }

    case P4Parser::token_type::TOK_INTEGER:
        {
            UnparsedConstant unparsedConst {
                cur->text,
                cur->constInfo->skip,
                cur->constInfo->base,
                cur->constInfo->hasWidth
            };
            auto& srcInfo = cur->srcInfo;
            return Token((P4Parser::token_type) cur->token_type, unparsedConst,
                         srcInfo);
        }

    default:
        return Token((P4Parser::token_type) cur->token_type,
                     P4::Token(cur->token_type, cur->text),
                     cur->srcInfo);
    }
}

}  // namespace P4
