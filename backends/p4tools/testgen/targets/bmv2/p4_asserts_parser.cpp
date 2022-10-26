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

/// The function to get kind from token
Token::Kind Token::kind() const noexcept { return m_kind; }
/// The function to replace the token kind with another kind
void Token::kind(Token::Kind kind) noexcept { m_kind = kind; }
/// Kind comparison function. Allows you to compare token kind with the specified kind and return
/// bool values, replacement for ==
bool Token::is(Token::Kind kind) const noexcept { return m_kind == kind; }
/// Kind comparison function. Allows you to compare token kind with the specified kind and return
/// bool values, replacement for !=
bool Token::is_not(Token::Kind kind) const noexcept { return m_kind != kind; }
/// Functions for multiple comparison kind
bool Token::is_one_of(Token::Kind k1, Token::Kind k2) const noexcept { return is(k1) || is(k2); }
template <typename... Ts>
bool Token::is_one_of(Token::Kind k1, Token::Kind k2, Ts... ks) const noexcept {
    return is(k1) || is_one_of(k2, ks...);
}
/// The function to get lexeme from token
std::string_view Token::lexeme() const noexcept { return m_lexeme; }
/// The function to replace the token lexeme with another lexeme
void Token::lexeme(std::string_view lexeme) noexcept { m_lexeme = lexeme; }

/// Function to get the current character
char Lexer::peek() const noexcept { return *m_beg; }
/// The function advances the iterator one index back and returns the character corresponding to the
/// position of the iterator
char Lexer::prev() noexcept { return *m_beg--; }
/// The function advances the iterator one index forward and returns the character corresponding to
/// the position of the iterator
char Lexer::get() noexcept { return *m_beg++; }

bool isSpace(char c) noexcept {
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
    static const char* const NAMES[]{
        "Priority",    "Text",           "True",         "False",       "LineStatementClose",
        "Id",          "Number",         "Minus",        "Plus",        "Dot",
        "FieldAcces",  "MetadataAccess", "LeftParen",    "RightParen",  "Equal",
        "NotEqual",    "GreaterThan",    "GreaterEqual", "LessThan",    "LessEqual",
        "LNot",        "Colon",          "Semicolon",    "Conjunction", "Disjunction",
        "Implication", "Slash",          "Percent",      "Shr",         "Shl",
        "Mul",         "Comment",        "Unknown",      "EndString",   "End",
    };
    return os << NAMES[static_cast<int>(kind)];
}

/// The function combines IR expressions separated by the "or" ,"and" operators
/// and also by the implication which is indicated by "(tmp" ,inside the algorithm,
/// for the convenience of determining the order of expressions. At the output, we get one
/// expression. For example, at the input we have a vector of expressions: [IR::Expression, IR::LOr
/// with name "(tmp", IR::Expression, IR::LAnd, IR::Expression] The result will be an IR::Expression
/// equal to !IR::Expression || (IR::Expression && IR::Expression)
const IR::Expression* makeSingleExpr(std::vector<const IR::Expression*> input) {
    const IR::Expression* expr = nullptr;
    for (uint64_t i = 0; i < input.size(); i++) {
        if (const auto* lOr = input[i]->to<IR::LOr>()) {
            if (lOr->right->toString() == "(tmp") {
                if (i + 1 == input.size()) {
                    break;
                }
                std::vector<const IR::Expression*> tmp;
                uint64_t idx = i;
                i++;
                if (i + 1 == input.size()) {
                    expr = new IR::LOr(input[idx - 1], input[i]);
                    break;
                }
                while (i < input.size()) {
                    tmp.push_back(input[i]);
                    i++;
                }
                if (expr == nullptr) {
                    expr = new IR::LOr(input[idx - 1], makeSingleExpr(tmp));
                } else {
                    expr = new IR::LOr(expr, makeSingleExpr(tmp));
                }
                break;
            }
            {
                if (i + 1 == input.size()) {
                    break;
                }
                if (expr == nullptr) {
                    expr = new IR::LOr(input[i - 1], input[i + 1]);
                } else {
                    expr = new IR::LOr(expr, input[i + 1]);
                }
            }
        }
        if (input[i]->is<IR::LAnd>()) {
            if (i + 1 == input.size()) {
                break;
            }
            if (expr == nullptr) {
                expr = new IR::LAnd(input[i - 1], input[i + 1]);
            } else {
                expr = new IR::LAnd(expr, input[i + 1]);
            }
        }
    }

    return expr;
}

/// Determines the token type according to the table key and generates a zombie constant for it.
const IR::Expression* makeConstant(Token input, const IR::Vector<IR::KeyElement>& keyElements,
                                   const IR::Type* leftType) {
    const IR::Type_Base* type = nullptr;
    const IR::Expression* result = nullptr;
    auto inputStr = input.lexeme();
    if (input.is(Token::Kind::Text)) {
        for (const auto* key : keyElements) {
            cstring annotationName;
            if (const auto* annotation = key->getAnnotation("name")) {
                if (!annotation->body.empty()) {
                    annotationName = annotation->body[0]->text;
                }
            }
            BUG_CHECK(annotationName.size() > 0, "Key does not have a name annotation.");
            auto annoSize = annotationName.size();
            auto tokenLength = inputStr.length();
            if (inputStr.find(annotationName, tokenLength - annoSize) == std::string::npos) {
                continue;
            }
            const auto* keyType = key->expression->type;
            if (const auto* bit = keyType->to<IR::Type_Bits>()) {
                type = bit;
            } else if (const auto* varbit = keyType->to<IR::Extracted_Varbits>()) {
                type = varbit;
            } else if (keyType->is<IR::Type_Boolean>()) {
                type = IR::Type_Bits::get(1);
            } else {
                BUG("Unexpected key type %s.", type->node_type_name());
            }
            return IRUtils::getZombieConst(type, 0, std::string(inputStr));
        }
    }
    if (input.is(Token::Kind::Number)) {
        if (leftType == nullptr)
            return IRUtils::getConstant(IR::Type_Bits::get(32), static_cast<big_int>(inputStr));
        else
            return IRUtils::getConstant(leftType, static_cast<big_int>(inputStr));
    }
    // TODO: Is this the right solution for priorities?
    if (input.is(Token::Kind::Priority)) {
        return IRUtils::getZombieConst(IR::Type_Bits::get(32), 0, std::string(inputStr));
    }
    BUG_CHECK(result != nullptr,
              "Could not match restriction key label %s was not found in key list.",
              std::string(inputStr));
    return nullptr;
}

/// Determines the right side of the expression starting from the original position and returns a
/// slice of tokens related to the right side and the index of the end of the right side.
/// Returning the end index is necessary so that after moving from the end of the right side
std::pair<std::vector<Token>, int> findRightPart(std::vector<Token> tokens, int index) {
    int idx = index + 1;
    int endIdx = 0;
    bool flag = true;
    while (flag) {
        if (idx == (int)tokens.size() ||
            tokens[idx].is_one_of(Token::Kind::Conjunction, Token::Kind::Disjunction,
                                  Token::Kind::RightParen)) {
            endIdx = idx;
            flag = false;
        }
        idx++;
    }

    std::vector<Token> rightTokens;
    for (int j = index + 1; j < endIdx; j++) {
        rightTokens.push_back(tokens[j]);
    }
    return std::make_pair(rightTokens, idx);
}

/// Converts a vector of tokens into a single IR:Expression
/// For example, at the input we have a vector of tokens:
/// [key1(Text), ->(Implication), key2(Text), &&(Conjunction), key3(Text)] The result will be an
/// IR::Expression equal to !IR::Expression || (IR::Expression && IR::Expression)
const IR::Expression* getIR(std::vector<Token> tokens,
                            const IR::Vector<IR::KeyElement>& keyElements) {
    const IR::Expression* expr = nullptr;
    std::vector<const IR::Expression*> exprVec;

    for (int i = 0; i < static_cast<int>(tokens.size()); i++) {
        auto token = tokens.at(i);
        if (token.is_one_of(
                Token::Kind::Minus, Token::Kind::Plus, Token::Kind::Equal, Token::Kind::NotEqual,
                Token::Kind::GreaterThan, Token::Kind::GreaterEqual, Token::Kind::LessThan,
                Token::Kind::LessEqual, Token::Kind::Slash, Token::Kind::Percent, Token::Kind::Shr,
                Token::Kind::Shl, Token::Kind::Mul, Token::Kind::NotEqual)) {
            const IR::Expression* leftL = nullptr;
            const IR::Expression* rightL = nullptr;
            leftL = makeConstant(tokens[i - 1], keyElements, nullptr);
            if (tokens[i + 1].is_one_of(Token::Kind::Text, Token::Kind::Number)) {
                rightL = makeConstant(tokens[i + 1], keyElements, leftL->type);
                if (const auto* constant = leftL->to<IR::Constant>()) {
                    auto* clone = constant->clone();
                    clone->type = rightL->type;
                    leftL = clone;
                }
            } else {
                auto rightPart = findRightPart(tokens, i);
                rightL = getIR(rightPart.first, keyElements);
                i = rightPart.second;
            }

            if (i - 2 > 0 && tokens[i - 2].is(Token::Kind::LNot)) {
                leftL = new IR::LNot(leftL);
            }

            if (tokens[i].is(Token::Kind::Minus)) expr = new IR::Sub(leftL, rightL);

            if (tokens[i].is(Token::Kind::Plus)) expr = new IR::Add(leftL, rightL);

            if (tokens[i].is(Token::Kind::Equal)) expr = new IR::Equ(leftL, rightL);

            if (tokens[i].is(Token::Kind::NotEqual)) expr = new IR::Neq(leftL, rightL);

            if (tokens[i].is(Token::Kind::GreaterThan)) expr = new IR::Grt(leftL, rightL);

            if (tokens[i].is(Token::Kind::GreaterEqual)) expr = new IR::Geq(leftL, rightL);

            if (tokens[i].is(Token::Kind::LessThan)) expr = new IR::Lss(leftL, rightL);

            if (tokens[i].is(Token::Kind::LessEqual)) expr = new IR::Leq(leftL, rightL);

            if (tokens[i].is(Token::Kind::Slash)) expr = new IR::Div(leftL, rightL);

            if (tokens[i].is(Token::Kind::Percent)) expr = new IR::Mod(leftL, rightL);

            if (tokens[i].is(Token::Kind::Shr)) expr = new IR::Shr(leftL, rightL);

            if (tokens[i].is(Token::Kind::Shl)) expr = new IR::Shl(leftL, rightL);

            if (tokens[i].is(Token::Kind::Mul)) expr = new IR::Mul(leftL, rightL);

            if (tokens[i].is(Token::Kind::NotEqual)) expr = new IR::Neq(leftL, rightL);

            exprVec.push_back(expr);

        } else if (token.is(Token::Kind::LNot)) {
            if (!tokens[i + 1].is_one_of(Token::Kind::Text, Token::Kind::Number)) {
                auto rightPart = findRightPart(tokens, i);
                const IR::Expression* exprLNot = getIR(rightPart.first, keyElements);
                i = rightPart.second;
                exprVec.push_back(new IR::LNot(exprLNot));
            }
        } else if (tokens[i].is(Token::Kind::Implication)) {
            auto tmp = exprVec[exprVec.size() - 1];
            exprVec.pop_back();
            const IR::Expression* expr1 = new IR::PathExpression(new IR::Path(IR::ID("tmp")));
            exprVec.push_back(new IR::LNot(tmp));
            const IR::Expression* expr2 = new IR::PathExpression(new IR::Path(IR::ID("(tmp")));
            exprVec.push_back(new IR::LOr(expr1, expr2));
        } else if (tokens[i].is(Token::Kind::Disjunction)) {
            const IR::Expression* expr1 = new IR::PathExpression(new IR::Path(IR::ID("tmp")));
            const IR::Expression* expr2 = new IR::PathExpression(new IR::Path(IR::ID("tmp")));
            exprVec.push_back(new IR::LOr(expr1, expr2));
        } else if (tokens[i].is(Token::Kind::Conjunction)) {
            const IR::Expression* expr1 = new IR::PathExpression(new IR::Path(IR::ID("tmp")));
            const IR::Expression* expr2 = new IR::PathExpression(new IR::Path(IR::ID("tmp")));
            exprVec.push_back(new IR::LAnd(expr1, expr2));
        }
    }

    if (exprVec.size() == 1) {
        return exprVec[0];
    }

    expr = makeSingleExpr(exprVec);

    return expr;
}
/// Combines successive tokens into variable names.
/// Returns a vector with tokens combined into names.
/// For example, at the input we have a vector of tokens:
/// [a(Text),c(Text),b(text), +(Plus), 1(Number),2(number)]
/// The result will be [acb(Text), +(Plus), 1(Number),2(number)]
std::vector<Token> combineTokensToNames(const std::vector<Token>& inputVector) {
    std::vector<Token> result;
    Token prevToken = Token(Token::Kind::Unknown, " ", 1);
    cstring txt = "";
    for (const auto& input : inputVector) {
        if (prevToken.is(Token::Kind::Text) && input.is(Token::Kind::Number)) {
            txt += std::string(input.lexeme());
            continue;
        }
        if (input.is(Token::Kind::Text)) {
            auto strtmp = std::string(input.lexeme());
            if (strtmp == "." && prevToken.is(Token::Kind::Number)) {
                ::error(
                    "Syntax error, unexpected INTEGER. P4 does not support floating point values. "
                    "Exiting");
            }
            txt += strtmp;
            if (prevToken.is(Token::Kind::Number)) {
                txt = std::string(prevToken.lexeme()) + txt;
                result.pop_back();
            }
        } else {
            if (txt.size() > 0) {
                result.emplace_back(Token::Kind::Text, txt, txt.size());
                txt = "";
            }
            result.push_back(input);
        }

        prevToken = input;
    }
    return result;
}

/// Combines successive tokens into numbers. For converting boolean or Hex, Octal values ​​to
/// numbers we must first use combineTokensToNames. Returns a vector with tokens combined into
/// numbers. For example, at the input we have a vector of tokens: [a(Text),c(Text),b(text),
/// +(Plus), 1(Number),2(number)] The result will be [a(Text),c(Text),b(text), +(Plus), 12(number)]
/// Other examples :
/// [acb(text), ||(Disjunction), true(text)] -> [acb(text), ||(Disjunction), 1(number)] - This
/// example is possible only after executing combineTokensToNames, since to convert bool values,
/// they must first be collected from text tokens in the combineTokensToNames function
/// [2(Number), 5(number), 5(number), ==(Equal), 0xff(Text)] -> [255(number), ==(Equal),
/// 0xff(Number)] - This example is possible only after executing combineTokensToNames
std::vector<Token> combineTokensToNumbers(std::vector<Token> input) {
    cstring numb = "";
    std::vector<Token> result;
    for (uint64_t i = 0; i < input.size(); i++) {
        if (input[i].is(Token::Kind::Text)) {
            auto str = std::string(input[i].lexeme());

            if (str.rfind("0x", 0) == 0 || str.rfind("0X", 0) == 0) {
                cstring cstr = str;
                result.emplace_back(Token::Kind::Number, cstr, cstr.size());
                continue;
            }

            if (str == "true") {
                result.emplace_back(Token::Kind::Number, "1", 1);
                continue;
            }

            if (str == "false") {
                result.emplace_back(Token::Kind::Number, "0", 1);
                continue;
            }
        }
        if (input[i].is(Token::Kind::Number)) {
            numb += std::string(input[i].lexeme());
            if (i + 1 == input.size()) {
                result.emplace_back(Token::Kind::Number, numb, numb.size());
                numb = "";
                continue;
            }
        } else {
            if (numb.size() > 0) {
                result.emplace_back(Token::Kind::Number, numb, numb.size());
                numb = "";
            }
            result.push_back(input[i]);
        }
    }
    return result;
}
/// Convert access tokens or keys into table keys. For converting access tokens or keys ​​to
/// table keys we must first use combineTokensToNames. Returns a vector with tokens converted into
/// table keys.
/// For example, at the input we have a vector of tokens:
/// [key::mask(Text), ==(Equal) , 0(Number)] The result will be [tableName_mask_key(Text), ==(Equal)
/// , 0(Number)] Other examples : [key::prefix_length(Text), ==(Equal) , 0(Number)] ->
/// [tableName_lpm_prefix_key(Text), ==(Equal) , 0(Number)] [key::priority(Text), ==(Equal) ,
/// 0(Number)] -> [tableName_priority_key(Text), ==(Equal) , 0(Number)] [Name01(Text), ==(Equal) ,
/// 0(Number)] -> [tableName_key_Name01(Text), ==(Equal) , 0(Number)]
std::vector<Token> combineTokensToTableKeys(std::vector<Token> input, cstring tableName) {
    std::vector<Token> result;
    for (uint64_t i = 0; i < input.size(); i++) {
        if (!input[i].is(Token::Kind::Text)) {
            result.push_back(input[i]);
            continue;
        }
        auto str = std::string(input[i].lexeme());

        auto substr = str.substr(0, str.find("::mask"));
        if (substr != str) {
            cstring cstr = tableName + "_mask_" + substr;
            result.emplace_back(Token::Kind::Text, cstr, cstr.size());
            continue;
        }
        substr = str.substr(0, str.find("::prefix_length"));
        if (substr != str) {
            cstring cstr = tableName + "_lpm_prefix_" + substr;
            result.emplace_back(Token::Kind::Text, cstr, cstr.size());
            continue;
        }

        substr = str.substr(0, str.find("::priority"));
        if (substr != str) {
            cstring cstr = tableName + "_priority";
            result.emplace_back(Token::Kind::Priority, cstr, cstr.size());
            continue;
        }

        if (str.find("isValid") != std::string::npos) {
            i++;
            cstring cString = str + std::string(input[i].lexeme());
            if (input[i + 1].is(Token::Kind::Text)) {
                cString += std::string(input[i + 1].lexeme());
                i++;
            }
            cString += std::string(input[i + 1].lexeme());
            i++;
            cString = tableName + "_key_" + cString;
            result.emplace_back(Token::Kind::Text, cString, cString.size());
            continue;
        }

        cstring cstr = tableName + "_key_" + str;
        result.emplace_back(Token::Kind::Text, cstr, cstr.size());
    }
    return result;
}

/// Removes comment lines from the input array.
/// For example, at the input we have a vector of tokens:
/// [//(Comment), CommentText(Text) , (EndString) , 28(Number), .....] -> [28(Number), .....]
std::vector<Token> removeComments(const std::vector<Token>& input) {
    std::vector<Token> result;
    bool flag = true;
    for (const auto& i : input) {
        if (i.is(Token::Kind::Comment)) {
            flag = false;
            continue;
        }
        if (i.is(Token::Kind::EndString)) {
            flag = true;
            continue;
        }
        if (flag) {
            result.push_back(i);
        }
    }
    return result;
}

/// A function that calls the beginning of the transformation of restrictions from a string into an
/// IR::Expression. Internally calls all other necessary functions, for example combineTokensToNames
/// and the like, to eventually get an IR expression that meets the string constraint
std::vector<const IR::Expression*> AssertsParser::genIRStructs(
    cstring tableName, cstring restrictionString, const IR::Vector<IR::KeyElement>& keyElements) {
    Lexer lex(restrictionString);
    std::vector<Token> tmp;
    for (auto token = lex.next(); !token.is_one_of(Token::Kind::End, Token::Kind::Unknown);
         token = lex.next()) {
        tmp.push_back(token);
    }

    std::vector<const IR::Expression*> result;

    tmp = combineTokensToNames(tmp);
    tmp = combineTokensToNumbers(tmp);
    tmp = combineTokensToTableKeys(tmp, tableName);
    tmp = removeComments(tmp);
    std::vector<Token> tokens;
    for (uint64_t i = 0; i < tmp.size(); i++) {
        if (tmp[i].is(Token::Kind::Semicolon)) {
            const auto* expr = getIR(tokens, keyElements);
            result.push_back(expr);
            tokens.clear();
        } else if (i == tmp.size() - 1) {
            tokens.push_back(tmp[i]);
            const auto* expr = getIR(tokens, keyElements);
            result.push_back(expr);
            tokens.clear();
        } else {
            tokens.push_back(tmp[i]);
        }
    }

    return result;
}

const IR::Node* AssertsParser::postorder(IR::P4Table* node) {
    const auto* annotation = node->getAnnotation("entry_restriction");
    const auto* key = node->getKey();
    if (annotation == nullptr || key == nullptr) {
        return node;
    }

    for (const auto* restrStr : annotation->body) {
        auto restrictions =
            genIRStructs(node->controlPlaneName(), restrStr->text, key->keyElements);
        /// Using Z3Solver, we check the feasibility of restrictions, if they are not
        /// feasible, we delete keys and entries from the table to execute
        /// default_action
        Z3Solver solver;
        if (solver.checkSat(restrictions) == true) {
            restrictionsVec.push_back(restrictions);
            continue;
        }
        auto* cloneTable = node->clone();
        auto* cloneProperties = node->properties->clone();
        IR::IndexedVector<IR::Property> properties;
        for (const auto* property : cloneProperties->properties) {
            if (property->name.name != "key" || property->name.name != "entries") {
                properties.push_back(property);
            }
        }
        cloneProperties->properties = properties;
        cloneTable->properties = cloneProperties;
        return cloneTable;
    }
    return node;
}

Token Lexer::atom(Token::Kind kind) noexcept { return {kind, m_beg++, 1}; }

Token Lexer::next() noexcept {
    while (isSpace(peek())) {
        get();
    }

    switch (peek()) {
        case '\0':
            return {Token::Kind::End, m_beg, 1};
        case '\n':
            get();
            return {Token::Kind::EndString, m_beg, 1};
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
                return {Token::Kind::Equal, "==", 2};
            }
            prev();
            return atom(Token::Kind::Unknown);
        case '!':
            get();
            if (get() == '=') {
                get();
                return {Token::Kind::NotEqual, "!=", 2};
            }
            prev();
            prev();
            return atom(Token::Kind::LNot);
        case '-':
            get();
            if (get() == '>') {
                get();
                return {Token::Kind::Implication, "->", 2};
            }
            prev();
            return atom(Token::Kind::Minus);
        case '<':
            get();
            if (get() == '=') {
                get();
                return {Token::Kind::LessEqual, "<=", 2};
            }
            prev();
            if (get() == '<') {
                return {Token::Kind::Shl, "<<", 2};
            }
            prev();
            return atom(Token::Kind::LessThan);
        case '>':
            get();
            if (get() == '=') {
                get();
                return {Token::Kind::GreaterEqual, ">=", 2};
            }
            prev();
            if (get() == '>') {
                return {Token::Kind::Shr, ">>", 2};
            }
            prev();
            return atom(Token::Kind::GreaterThan);
        case ';':
            return atom(Token::Kind::Semicolon);
        case '&':
            get();
            if (get() == '&') {
                get();
                return {Token::Kind::Conjunction, "&&", 2};
            }
            prev();
            return atom(Token::Kind::Text);
        case '|':
            get();
            if (get() == '|') {
                get();
                return {Token::Kind::Disjunction, "||", 2};
            }
            prev();
            return atom(Token::Kind::Text);
        case '+':
            return atom(Token::Kind::Plus);
        case '/':
            get();
            if (get() == '/') {
                get();
                return {Token::Kind::Comment, "//", 2};
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
