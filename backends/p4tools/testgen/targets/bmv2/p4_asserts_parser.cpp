#include "backends/p4tools/testgen/targets/bmv2/p4_asserts_parser.h"

#include <boost/multiprecision/cpp_int.hpp>
#include <boost/optional/optional_io.hpp>

#include "backends/p4tools/common/core/z3_solver.h"
#include "backends/p4tools/common/lib/ir.h"
#include "lib/error.h"

namespace P4Tools {

namespace AssertsParser {

AssertsParser::AssertsParser(std::vector<std::vector<const IR::Expression*>>& output)
    : restrictionsVec(output) {
    setName("Restrictions");
}

bool is_space(char c) noexcept {
    switch (c) {
        case ' ':
        case '\t':
        case '\r':
            return true;
        default:
            return false;
    }
}

std::ostream& operator<<(std::ostream& os, const Token::Kind& kind) {
    static const char* const names[]{
        "Text",         "True",       "False",          "LineStatementClose",
        "Id",           "Number",     "Minus",          "Plus",
        "Dot",          "FieldAcces", "MetadataAccess", "LeftParen",
        "RightParen",   "Equal",      "NotEqual",       "GreaterThan",
        "GreaterEqual", "LessThan",   "LessEqual",      "LNot",
        "Colon",        "Semicolon",  "Conjunction",    "Disjunction",
        "Implication",  "Slash",      "Percent",        "Shr",
        "Shl",          "Mul",        "Comment",        "Unknown",
        "EndString",    "End",
    };
    return os << names[static_cast<int>(kind)];
}

const IR::Expression* makeExpr(std::vector<const IR::Expression*> input) {
    const IR::Expression* expr = nullptr;
    for (long unsigned int i = 0; i < input.size(); i++) {
        if (auto lOr = input[i]->to<IR::LOr>()) {
            if (expr == nullptr) {
                if (lOr->right->toString() == "(tmp") {
                    std::vector<const IR::Expression*> tmp;
                    long unsigned int idx = i;
                    i++;
                    if (i + 1 == input.size()) {
                        expr = new IR::LOr(input[idx - 1], input[i]);
                        break;
                    }
                    while (i < input.size()) {
                        tmp.push_back(input[i]);
                        i++;
                    }
                    expr = new IR::LOr(input[idx - 1], makeExpr(tmp));
                    break;
                }
                expr = new IR::LOr(input[i - 1], input[i + 1]);
                continue;
            } else if (lOr->right->toString() == "(tmp") {
                if (i + 1 != input.size()) {
                    std::vector<const IR::Expression*> tmp;
                    i++;
                    if (i + 1 == input.size()) {
                        expr = new IR::LOr(input[i - 2], input[i]);
                    }
                    while (i < input.size()) {
                        tmp.push_back(input[i]);
                        i++;
                    }
                    expr = new IR::LOr(expr, makeExpr(tmp));
                }

            } else {
                if (i + 1 != input.size()) {
                    expr = new IR::LOr(expr, input[i + 1]);
                }
            }
        }
        if (input[i]->is<IR::LAnd>()) {
            if (expr == nullptr) {
                expr = new IR::LAnd(input[i - 1], input[i + 1]);
                continue;
            } else {
                if (i + 1 != input.size()) expr = new IR::LAnd(expr, input[i + 1]);
            }
        }
    }

    return expr;
}
const IR::Expression* getIR(std::vector<Token> tokens, IR::Vector<IR::KeyElement> keyElements) {
    const IR::Expression* expr = nullptr;
    std::vector<const IR::Expression*> exprVec;
    const IR::Type_Base* type = nullptr;

    for (long unsigned int i = 0; i < tokens.size(); i++) {
        if (tokens[i].kind() == Token::Kind::Minus || tokens[i].kind() == Token::Kind::Plus ||
            tokens[i].kind() == Token::Kind::Equal || tokens[i].kind() == Token::Kind::NotEqual ||
            tokens[i].kind() == Token::Kind::GreaterThan ||
            tokens[i].kind() == Token::Kind::GreaterEqual ||
            tokens[i].kind() == Token::Kind::LessThan ||
            tokens[i].kind() == Token::Kind::LessEqual || tokens[i].kind() == Token::Kind::Slash ||
            tokens[i].kind() == Token::Kind::Percent || tokens[i].kind() == Token::Kind::Shr ||
            tokens[i].kind() == Token::Kind::Shl || tokens[i].kind() == Token::Kind::Mul ||
            tokens[i].kind() == Token::Kind::NotEqual) {
            const IR::Expression* leftL = nullptr;
            const IR::Expression* rightL = nullptr;
            if (i != 0) {
                i--;
                i--;
            }
            if (i <= 0 || tokens[i].kind() == Token::Kind::Conjunction ||
                tokens[i].kind() == Token::Kind::Disjunction ||
                tokens[i].kind() == Token::Kind::LeftParen ||
                tokens[i].kind() == Token::Kind::Implication) {
                i++;

                if (tokens[i].kind() == Token::Kind::Text) {
                    std::string str = static_cast<std::string>(tokens[i].lexeme());
                    for (auto key : keyElements) {
                        cstring annotationName = "";
                        for (auto annotation : key->annotations->annotations) {
                            if (annotation->name.name == "name") {
                                if (annotation->body.size() > 0)
                                    annotationName = annotation->body[0]->text;
                            }
                        }
                        if (str.find(key->expression->toString()) != std::string::npos ||
                            (annotationName.size() > 0 &&
                             str.find(annotationName) != std::string::npos)) {
                            if (auto bit = key->expression->type->to<IR::Type_Bits>()) {
                                type = bit;
                            } else if (auto varbit =
                                           key->expression->type->to<IR::Extracted_Varbits>()) {
                                type = varbit;
                            } else if (key->expression->type->to<IR::Type_Unknown>()) {
                                type = IR::Type_Bits::get(32);
                            } else {
                                type = IR::Type_Bits::get(1);
                            }
                            leftL = IRUtils::getZombieConst(type, 0, str);
                            break;
                        }
                    }
                    if (!leftL) {
                        type = IR::Type_Bits::get(8);
                        leftL = IRUtils::getZombieConst(type, 0, str);
                    }

                } else if (tokens[i].kind() == Token::Kind::Number) {
                    if (!type) {
                        type = IR::Type_Bits::get(8);
                    }
                    std::string str = static_cast<std::string>(tokens[i].lexeme());
                    leftL = IRUtils::getConstant(type, static_cast<big_int>(str));
                }
            } else {
                leftL = exprVec[exprVec.size() - 1];
                exprVec.pop_back();
                i++;
            }
            i++;
            i++;
            i++;
            if (i == tokens.size() || tokens[i].kind() == Token::Kind::Conjunction ||
                tokens[i].kind() == Token::Kind::Disjunction ||
                tokens[i].kind() == Token::Kind::RightParen ||
                tokens[i].kind() == Token::Kind::Implication) {
                i--;
                if (tokens[i].kind() == Token::Kind::Text) {
                    std::string str = static_cast<std::string>(tokens[i].lexeme());
                    for (auto key : keyElements) {
                        cstring annotationName = "";
                        for (auto annotation : key->annotations->annotations) {
                            if (annotation->name.name == "name") {
                                if (annotation->body.size() > 0)
                                    annotationName = annotation->body[0]->text;
                            }
                        }
                        if (str.find(key->expression->toString()) != std::string::npos ||
                            (annotationName.size() > 0 &&
                             str.find(annotationName) != std::string::npos)) {
                            if (auto bit = key->expression->type->to<IR::Type_Bits>()) {
                                type = bit;
                            } else if (auto varbit =
                                           key->expression->type->to<IR::Extracted_Varbits>()) {
                                type = varbit;
                            } else if (key->expression->type->to<IR::Type_Unknown>()) {
                                type = IR::Type_Bits::get(32);
                            } else {
                                type = IR::Type_Bits::get(1);
                            }
                            if (auto constant = leftL->to<IR::Constant>()) {
                                auto clone = constant->clone();
                                clone->type = type;
                                leftL = clone;
                            }
                            rightL = IRUtils::getZombieConst(type, 0, str);
                            break;
                        }
                    }
                    if (!rightL) {
                        type = IR::Type_Bits::get(8);
                        rightL = IRUtils::getZombieConst(type, 0, str);
                    }
                } else if (tokens[i].kind() == Token::Kind::Number) {
                    if (!type) {
                        type = IR::Type_Bits::get(8);
                    }
                    std::string str = static_cast<std::string>(tokens[i].lexeme());
                    rightL = IRUtils::getConstant(type, static_cast<big_int>(str));
                }
            } else {
                i--;
                int idx = i;
                int endIdx = 0;
                bool flag = true;
                while (flag) {
                    i++;
                    if (i == tokens.size() || tokens[i].kind() == Token::Kind::Conjunction ||
                        tokens[i].kind() == Token::Kind::Disjunction ||
                        tokens[i].kind() == Token::Kind::RightParen) {
                        endIdx = i;
                        flag = false;
                        i = idx;
                    }
                }

                std::vector<Token> rightTokens;
                for (int j = idx; j < endIdx + 1; j++) {
                    rightTokens.push_back(tokens[j]);
                }

                rightL = getIR(rightTokens, keyElements);
            }
            i--;
            if (tokens[i].kind() == Token::Kind::Minus) expr = new IR::Sub(leftL, rightL);
            if (tokens[i].kind() == Token::Kind::Plus) expr = new IR::Add(leftL, rightL);
            if (tokens[i].kind() == Token::Kind::Equal) expr = new IR::Equ(leftL, rightL);
            if (tokens[i].kind() == Token::Kind::NotEqual) expr = new IR::Neq(leftL, rightL);
            if (tokens[i].kind() == Token::Kind::GreaterThan) expr = new IR::Grt(leftL, rightL);
            if (tokens[i].kind() == Token::Kind::GreaterEqual) expr = new IR::Geq(leftL, rightL);
            if (tokens[i].kind() == Token::Kind::LessThan) expr = new IR::Lss(leftL, rightL);
            if (tokens[i].kind() == Token::Kind::LessEqual) expr = new IR::Leq(leftL, rightL);
            if (tokens[i].kind() == Token::Kind::Slash) expr = new IR::Div(leftL, rightL);
            if (tokens[i].kind() == Token::Kind::Percent) expr = new IR::Mod(leftL, rightL);
            if (tokens[i].kind() == Token::Kind::Shr) expr = new IR::Shr(leftL, rightL);
            if (tokens[i].kind() == Token::Kind::Shl) expr = new IR::Shl(leftL, rightL);
            if (tokens[i].kind() == Token::Kind::Mul) expr = new IR::Mul(leftL, rightL);
            if (tokens[i].kind() == Token::Kind::NotEqual) expr = new IR::Neq(leftL, rightL);
            exprVec.push_back(expr);

        } else if (tokens[i].kind() == Token::Kind::LNot) {
            auto flag = true;
            int idx = i;
            cstring str = "";
            auto first = true;
            while (flag) {
                i++;
                if (tokens[i].kind() == Token::Kind::LeftParen) {
                    first = false;
                    continue;
                } else if (first) {
                    break;
                }
                if (tokens[i].kind() != Token::Kind::RightParen) {
                    str += static_cast<std::string>(tokens[i].lexeme());
                } else {
                    flag = false;
                }
            }
            i = idx;
            if (str.size() == 0) {
                str = "tmp";
            }
            const IR::Expression* expr1 = new IR::PathExpression(new IR::Path(IR::ID(str)));
            exprVec.push_back(new IR::LNot(expr1));
        } else if (tokens[i].kind() == Token::Kind::Implication) {
            auto tmp = exprVec[exprVec.size() - 1];
            exprVec.pop_back();
            const IR::Expression* expr1 = new IR::PathExpression(new IR::Path(IR::ID("tmp")));
            exprVec.push_back(new IR::LNot(expr1));
            exprVec.push_back(tmp);
            const IR::Expression* expr2 = new IR::PathExpression(new IR::Path(IR::ID("(tmp")));
            exprVec.push_back(new IR::LOr(expr1, expr2));
        } else if (tokens[i].kind() == Token::Kind::Disjunction) {
            const IR::Expression* expr1 = new IR::PathExpression(new IR::Path(IR::ID("tmp")));
            const IR::Expression* expr2 = new IR::PathExpression(new IR::Path(IR::ID("tmp")));
            exprVec.push_back(new IR::LOr(expr1, expr2));
        } else if (tokens[i].kind() == Token::Kind::Conjunction) {
            const IR::Expression* expr1 = new IR::PathExpression(new IR::Path(IR::ID("tmp")));
            const IR::Expression* expr2 = new IR::PathExpression(new IR::Path(IR::ID("tmp")));
            exprVec.push_back(new IR::LAnd(expr1, expr2));
        }
    }
    std::vector<const IR::Expression*> tmp;
    for (long unsigned int i = 0; i < exprVec.size(); i++) {
        if (auto lNot = exprVec[i]->to<IR::LNot>()) {
            i++;
            auto lNotStr = lNot->expr->toString();

            auto binary = exprVec[i]->checkedTo<IR::Operation_Binary>()->clone();
            if (auto subBin = binary->left->to<IR::Operation_Binary>()) {
                cstring str = subBin->getStringOp();
                str.replace(" ", "");

                if (lNotStr.find(str)) {
                    binary->left = new IR::LNot(binary->left);
                    tmp.push_back(binary);
                    continue;
                }
            }

            tmp.push_back(new IR::LNot(exprVec[i]));
            continue;
        } else {
            tmp.push_back(exprVec[i]);
        }
    }
    if (tmp.size() == 1) {
        return expr;
    }

    expr = makeExpr(tmp);

    return expr;
}

std::vector<const IR::Expression*> AssertsParser::genIRStructs(cstring str) {
    Lexer lex(str);
    std::vector<Token> tmp;
    std::vector<const IR::Expression*> result;
    Token prevToken = Token(Token::Kind::Unknown, " ", 1);
    cstring txt = "";
    cstring numb = "";
    for (auto token = lex.next(); !token.is_one_of(Token::Kind::End, Token::Kind::Unknown);
         token = lex.next()) {
        if (prevToken.kind() == Token::Kind::Text && token.kind() == Token::Kind::Number) {
            txt += static_cast<std::string>(token.lexeme());
            continue;
        }
        if (token.kind() == Token::Kind::Text) {
            auto strtmp = static_cast<std::string>(token.lexeme());
            if (strtmp == "." && prevToken.kind() == Token::Kind::Number) {
                ::error(
                    "Syntax error, unexpected INTEGER. P4 does not support floating point values. "
                    "Exiting");
            }
            txt += strtmp;
            if (prevToken.kind() == Token::Kind::Number) {
                txt = static_cast<std::string>(prevToken.lexeme()) + txt;
                tmp.pop_back();
            }
        } else {
            if (txt.size() > 0) {
                tmp.push_back(Token(Token::Kind::Text, txt, txt.size()));
                txt = "";
            }
            tmp.push_back(token);
        }

        prevToken = token;
    }
    std::vector<Token> tokens;
    prevToken = Token(Token::Kind::Unknown, " ", 1);
    for (long unsigned int i = 0; i < tmp.size(); i++) {
        if (tmp[i].kind() == Token::Kind::Text) {
            auto str = static_cast<std::string>(tmp[i].lexeme());

            if (str.rfind("0x", 0) == 0 || str.rfind("0X", 0) == 0) {
                cstring cstr = str;
                tokens.push_back(Token(Token::Kind::Number, cstr, cstr.size()));
                continue;
            }
            auto substr = str.substr(0, str.find("::mask"));
            if (substr != str) {
                cstring cstr = tableName + "_mask_" + substr;
                tokens.push_back(Token(Token::Kind::Text, cstr, cstr.size()));
                continue;
            }
            substr = str.substr(0, str.find("::prefix_length"));
            if (substr != str) {
                cstring cstr = tableName + "_lpm_prefix_" + substr;
                tokens.push_back(Token(Token::Kind::Text, cstr, cstr.size()));
                continue;
            }

            substr = str.substr(0, str.find("::priority"));
            if (substr != str) {
                cstring cstr = tableName + "_priority";
                tokens.push_back(Token(Token::Kind::Text, cstr, cstr.size()));
                continue;
            }

            if (str == "true") {
                tokens.push_back(Token(Token::Kind::Number, "1", 1));
                continue;
            }

            if (str == "false") {
                tokens.push_back(Token(Token::Kind::Number, "0", 1));
                continue;
            }

            if (str.find("isValid") != std::string::npos) {
                i++;
                cstring cString = str + static_cast<std::string>(tmp[i].lexeme());
                if (tmp[i + 1].kind() == Token::Kind::Text) {
                    cString += static_cast<std::string>(tmp[i + 1].lexeme());
                    i++;
                }
                cString += static_cast<std::string>(tmp[i + 1].lexeme());
                i++;
                cString = tableName + "_key_" + cString;
                tokens.push_back(Token(Token::Kind::Text, cString, cString.size()));
                continue;
            }

            cstring cstr = tableName + "_key_" + str;
            tokens.push_back(Token(Token::Kind::Text, cstr, cstr.size()));
            continue;
        }
        if (tmp[i].kind() == Token::Kind::Number) {
            numb += static_cast<std::string>(tmp[i].lexeme());
            if (i + 1 == tmp.size()) {
                tokens.push_back(Token(Token::Kind::Number, numb, numb.size()));
                numb = "";
                continue;
            }
        } else {
            if (numb.size() > 0) {
                tokens.push_back(Token(Token::Kind::Number, numb, numb.size()));
                numb = "";
            }
            tokens.push_back(tmp[i]);
        }
    }
    tmp.clear();
    bool flag = true;
    for (long unsigned int i = 0; i < tokens.size(); i++) {
        if (tokens[i].kind() == Token::Kind::Comment) {
            flag = false;
            continue;
        }
        if (tokens[i].kind() == Token::Kind::EndString) {
            flag = true;
            continue;
        }
        if (flag) {
            tmp.push_back(tokens[i]);
        }
    }
    tokens.clear();

    for (long unsigned int i = 0; i < tmp.size(); i++) {
        if (tmp[i].kind() == Token::Kind::Semicolon) {
            auto expr = getIR(tokens, keyElements);
            result.push_back(expr);
            tokens.clear();
        } else if (i == tmp.size() - 1) {
            tokens.push_back(tmp[i]);
            auto expr = getIR(tokens, keyElements);
            result.push_back(expr);
            tokens.clear();
        } else {
            tokens.push_back(tmp[i]);
        }
    }

    return result;
}

const IR::Node* AssertsParser::postorder(IR::P4Table* node) {
    tableName = node->controlPlaneName();
    if (node->getKey()) {
        keyElements = node->getKey()->keyElements;
        for (auto annotation : node->annotations->annotations) {
            if (annotation->name.name == "entry_restriction") {
                for (auto restrStr : annotation->body) {
                    auto restrictions = genIRStructs(restrStr->text);
                    Z3Solver solver;
                    if (solver.checkSat(restrictions) == true) {
                        restrictionsVec.push_back(restrictions);
                    } else {
                        auto cloneTable = node->clone();
                        auto cloneProperties = node->properties->clone();
                        IR::IndexedVector<IR::Property> properties;
                        for (auto property : cloneProperties->properties) {
                            if (property->name.name != "key" || property->name.name != "entries")
                                properties.push_back(property);
                        }
                        cloneProperties->properties = properties;
                        cloneTable->properties = cloneProperties;
                        return cloneTable;
                    }
                }
            }
        }
    }
    return node;
}

Token Lexer::atom(Token::Kind kind) noexcept { return Token(kind, m_beg++, 1); }

Token Lexer::next() noexcept {
    while (is_space(peek())) get();

    switch (peek()) {
        case '\0':
            return Token(Token::Kind::End, m_beg, 1);
        case '\n':
            get();
            return Token(Token::Kind::EndString, m_beg, 1);
        default:
            return atom(Token::Kind::Text);
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            return atom(Token::Kind::Number);
        case '(':
            return atom(Token::Kind::LeftParen);
        case ')':
            return atom(Token::Kind::RightParen);
        case '=':
            get();
            if (get() == '=') {
                get();
                return Token(Token::Kind::Equal, "==", 2);
            }
            prev();
            return atom(Token::Kind::Unknown);
        case '!':
            get();
            if (get() == '=') {
                get();
                return Token(Token::Kind::NotEqual, "!=", 2);
            }
            prev();
            return atom(Token::Kind::LNot);
        case '-':
            get();
            if (get() == '>') {
                get();
                return Token(Token::Kind::Implication, "->", 2);
            }
            prev();
            return atom(Token::Kind::Minus);
        case '<':
            get();
            if (get() == '=') {
                get();
                return Token(Token::Kind::LessEqual, "<=", 2);
            }
            prev();
            if (get() == '<') {
                return Token(Token::Kind::Shl, "<<", 2);
            }
            prev();
            return atom(Token::Kind::LessThan);
        case '>':
            get();
            if (get() == '=') {
                get();
                return Token(Token::Kind::GreaterEqual, ">=", 2);
            }
            prev();
            if (get() == '>') {
                return Token(Token::Kind::Shr, ">>", 2);
            }
            prev();
            return atom(Token::Kind::GreaterThan);
        case ';':
            return atom(Token::Kind::Semicolon);
        case '&':
            get();
            if (get() == '&') {
                get();
                return Token(Token::Kind::Conjunction, "&&", 2);
            }
            prev();
            return atom(Token::Kind::Text);
        case '|':
            get();
            if (get() == '|') {
                get();
                return Token(Token::Kind::Disjunction, "||", 2);
            }
            prev();
            return atom(Token::Kind::Text);
        case '+':
            return atom(Token::Kind::Plus);
        case '/':
            get();
            if (get() == '/') {
                get();
                return Token(Token::Kind::Comment, "//", 2);
            }
            prev();
            return atom(Token::Kind::Slash);
        case '%':
            return atom(Token::Kind::Percent);
        case '*':
            return atom(Token::Kind::Mul);
    }
}
}  // namespace AssertsParser

}  // namespace P4Tools
