#include "frontends/parsers/p4/p4AnnotationLexer.hpp"
#include "frontends/common/constantParsing.h"

namespace P4 {

P4AnnotationLexer::Token P4AnnotationLexer::yylex(P4::P4ParserDriver& driver) {
    // Driver not used here. Suppress compiler warning.
    (void) driver;

    if (needStart) {
        needStart = false;
        return P4Parser::symbol_type((P4Parser::token_type) type,
                                     Util::SourceInfo(body.srcInfo));
    }

    if (it == body.end()) {
        return P4Parser::make_END(Util::SourceInfo(body.srcInfo));
    }

    auto cur = *(it++);
    switch (cur->token_type) {
    case P4Parser::token_type::TOK_IDENTIFIER:
    case P4Parser::token_type::TOK_TYPE_IDENTIFIER:
        return Token((P4Parser::token_type) cur->token_type,
                     cstring(cur->text), Util::SourceInfo(cur->srcInfo));

    case P4Parser::token_type::TOK_INTEGER:
        {
            UnparsedConstant&& unparsedConst {
                cur->text,
                cur->constInfo->skip,
                cur->constInfo->base,
                cur->constInfo->hasWidth
            };
            return Token((P4Parser::token_type) cur->token_type, unparsedConst,
                         Util::SourceInfo(cur->srcInfo));
        }

    default:
        return Token((P4Parser::token_type) cur->token_type,
                     P4::Token(cur->token_type, cur->text),
                     Util::SourceInfo(cur->srcInfo));
    }
}

}  // namespace P4
