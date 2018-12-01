#ifndef FRONTENDS_P4_P4ANNOTATIONLEXER_H_
#define FRONTENDS_P4_P4ANNOTATIONLEXER_H_

#include "frontends/parsers/p4/abstractP4Lexer.hpp"
#include "frontends/parsers/p4/p4parser.hpp"

namespace P4 {

class P4AnnotationLexer : public AbstractP4Lexer {
 public:
    enum Type {
        EXPRESSION_LIST = P4Parser::token_type::TOK_START_EXPRESSION_LIST,
        KV_LIST = P4Parser::token_type::TOK_START_KV_LIST,
        EXPRESSION = P4Parser::token_type::TOK_START_EXPRESSION,
        INTEGER = P4Parser::token_type::TOK_START_INTEGER,
        STRING_LITERAL = P4Parser::token_type::TOK_START_STRING_LITERAL,
    };

 private:
    Type type;
    const IR::Vector<IR::AnnotationToken>& body;
    bool needStart;
    IR::Vector<IR::AnnotationToken>::const_iterator it;

 public:
    P4AnnotationLexer(Type type, const IR::Vector<IR::AnnotationToken>& body)
        : type(type), body(body), needStart(true), it(this->body.begin()) { }

    Token yylex(P4::P4ParserDriver& driver);
};

}  // namespace P4

#endif  /* FRONTENDS_P4_P4ANNOTATIONLEXER_H_ */
