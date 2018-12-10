#include "frontends/parsers/p4/p4AnnotationLexer.hpp"
#include "frontends/common/constantParsing.h"

namespace P4 {

UnparsedConstant unparsedConstant(const IR::AnnotationToken* token) {
    UnparsedConstant result {
        token->constInfo->text,
        token->constInfo->skip,
        token->constInfo->base,
        token->constInfo->hasWidth
    };

    return result;
}

P4AnnotationLexer::Token P4AnnotationLexer::yylex(P4::P4ParserDriver&) {
    if (needStart) {
        needStart = false;
        return P4Parser::symbol_type((P4Parser::token_type) type,
                                     Util::SourceInfo(srcInfo));
    }

    if (it == body.end()) {
        return P4Parser::make_END_ANNOTATION(Util::SourceInfo(srcInfo));
    }

    auto cur = *(it++);
    switch (cur->token_type) {
    case P4Parser::token_type::TOK_IDENTIFIER:
    case P4Parser::token_type::TOK_TYPE_IDENTIFIER:
    case P4Parser::token_type::TOK_STRING_LITERAL:
        return Token((P4Parser::token_type) cur->token_type,
                     cstring(cur->text), Util::SourceInfo(cur->srcInfo));

    case P4Parser::token_type::TOK_INTEGER:
        return Token((P4Parser::token_type) cur->token_type,
                     unparsedConstant(cur), Util::SourceInfo(cur->srcInfo));

    default:
        return Token((P4Parser::token_type) cur->token_type,
                     P4::Token(cur->token_type, cur->text),
                     Util::SourceInfo(cur->srcInfo));
    }
}

}  // namespace P4
