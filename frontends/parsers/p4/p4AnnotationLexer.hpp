#ifndef FRONTENDS_PARSERS_P4_P4ANNOTATIONLEXER_HPP_
#define FRONTENDS_PARSERS_P4_P4ANNOTATIONLEXER_HPP_

#include <string>
#include <vector>

#include "frontends/parsers/p4/abstractP4Lexer.hpp"
#include "frontends/parsers/p4/p4parser.hpp"
#include "ir/ir.h"
#include "ir/vector.h"
#include "lib/cstring.h"
#include "lib/source_file.h"

namespace P4 {

class P4AnnotationLexer : public AbstractP4Lexer {
 public:
    enum Type {
        // Lists
        EXPRESSION_LIST = P4Parser::token_type::TOK_START_EXPRESSION_LIST,
        KV_LIST = P4Parser::token_type::TOK_START_KV_LIST,
        INTEGER_LIST = P4Parser::token_type::TOK_START_INTEGER_LIST,
        INTEGER_OR_STRING_LITERAL_LIST =
            P4Parser::token_type::TOK_START_INTEGER_OR_STRING_LITERAL_LIST,
        STRING_LITERAL_LIST = P4Parser::token_type::TOK_START_STRING_LITERAL_LIST,

        // Singletons
        EXPRESSION = P4Parser::token_type::TOK_START_EXPRESSION,
        INTEGER = P4Parser::token_type::TOK_START_INTEGER,
        INTEGER_OR_STRING_LITERAL = P4Parser::token_type::TOK_START_INTEGER_OR_STRING_LITERAL,
        STRING_LITERAL = P4Parser::token_type::TOK_START_STRING_LITERAL,

        // Pairs
        EXPRESSION_PAIR = P4Parser::token_type::TOK_START_EXPRESSION_PAIR,
        INTEGER_PAIR = P4Parser::token_type::TOK_START_INTEGER_PAIR,
        STRING_LITERAL_PAIR = P4Parser::token_type::TOK_START_STRING_LITERAL_PAIR,

        // Triples
        EXPRESSION_TRIPLE = P4Parser::token_type::TOK_START_EXPRESSION_TRIPLE,
        INTEGER_TRIPLE = P4Parser::token_type::TOK_START_INTEGER_TRIPLE,
        STRING_LITERAL_TRIPLE = P4Parser::token_type::TOK_START_STRING_LITERAL_TRIPLE,

        // P4Runtime annotations
        P4RT_TRANSLATION_ANNOTATION = P4Parser::token_type::TOK_START_P4RT_TRANSLATION_ANNOTATION,
    };

 private:
    Type type;
    const IR::Vector<IR::AnnotationToken>& body;
    bool needStart;
    IR::Vector<IR::AnnotationToken>::const_iterator it;
    const Util::SourceInfo& srcInfo;

 public:
    P4AnnotationLexer(Type type, const Util::SourceInfo& srcInfo,
                      const IR::Vector<IR::AnnotationToken>& body)
        : type(type), body(body), needStart(true), it(this->body.begin()), srcInfo(srcInfo) {}

    Token yylex(P4::P4ParserDriver& driver);
};

}  // namespace P4

#endif /* FRONTENDS_PARSERS_P4_P4ANNOTATIONLEXER_HPP_ */
