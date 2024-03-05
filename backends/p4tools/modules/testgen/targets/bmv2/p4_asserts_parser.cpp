#include "backends/p4tools/modules/testgen/targets/bmv2/p4_asserts_parser.h"

#include <cstdint>
#include <iostream>
#include <optional>
#include <string>
#include <utility>

#include "backends/p4tools/common/core/z3_solver.h"
#include "backends/p4tools/common/lib/variables.h"
#include "ir/id.h"
#include "ir/indexed_vector.h"
#include "ir/ir.h"
#include "ir/irutils.h"
#include "lib/big_int_util.h"
#include "lib/error.h"
#include "lib/exceptions.h"

namespace P4Tools::P4Testgen::Bmv2 {

static const std::vector<std::string> NAMES{
    "Priority",    "Text",           "True",         "False",       "LineStatementClose",
    "Id",          "Number",         "Minus",        "Plus",        "Dot",
    "FieldAcces",  "MetadataAccess", "LeftParen",    "RightParen",  "Equal",
    "NotEqual",    "GreaterThan",    "GreaterEqual", "LessThan",    "LessEqual",
    "LNot",        "Colon",          "Semicolon",    "Conjunction", "Disjunction",
    "Implication", "Slash",          "Percent",      "Shr",         "Shl",
    "Mul",         "Comment",        "Unknown",      "EndString",   "End",
};

AssertsParser::AssertsParser(ConstraintsVector &output) : restrictionsVec(output) {
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
bool Token::isNot(Token::Kind kind) const noexcept { return m_kind != kind; }

/// Functions for multiple comparison kind
bool Token::isOneOf(Token::Kind k1, Token::Kind k2) const noexcept { return is(k1) || is(k2); }

template <typename... Ts>
bool Token::isOneOf(Token::Kind k1, Token::Kind k2, Ts... ks) const noexcept {
    return is(k1) || isOneOf(k2, ks...);
}

/// The function to get lexeme from token
std::string_view Token::lexeme() const noexcept { return m_lexeme; }

/// The function to replace the token lexeme with another lexeme
void Token::lexeme(std::string_view lexeme) noexcept { m_lexeme = lexeme; }

/// Function to get the current character
char Lexer::peek() const noexcept { return *mBeg; }

/// The function advances the iterator one index back and returns the character corresponding to the
/// position of the iterator
char Lexer::prev() noexcept { return *mBeg--; }

/// The function advances the iterator one index forward and returns the character corresponding to
/// the position of the iterator
char Lexer::get() noexcept { return *mBeg++; }

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

std::ostream &operator<<(std::ostream &os, const Token::Kind &kind) {
    return os << NAMES.at(static_cast<size_t>(kind));
}

/// The function combines IR expressions separated by the "or" ,"and" operators
/// and also by the implication which is indicated by "(tmp" ,inside the algorithm,
/// for the convenience of determining the order of expressions. At the output, we get one
/// expression. For example, at the input we have a vector of expressions: [IR::Expression, IR::LOr
/// with name "(tmp", IR::Expression, IR::LAnd, IR::Expression] The result will be an IR::Expression
/// equal to !IR::Expression || (IR::Expression && IR::Expression)
const IR::Expression *makeSingleExpr(std::vector<const IR::Expression *> input) {
    const IR::Expression *expr = nullptr;
    for (uint64_t idx = 0; idx < input.size(); idx++) {
        if (input[idx]->is<IR::LOr>()) {
            if (idx + 1 == input.size()) {
                break;
            }
            if (expr == nullptr) {
                expr = new IR::LOr(input[idx - 1], input[idx + 1]);
            } else {
                expr = new IR::LOr(expr, input[idx + 1]);
            }
        }
        if (input[idx]->is<IR::LAnd>()) {
            if (idx + 1 == input.size()) {
                break;
            }
            if (expr == nullptr) {
                expr = new IR::LAnd(input[idx - 1], input[idx + 1]);
            } else {
                expr = new IR::LAnd(expr, input[idx + 1]);
            }
        }
    }

    return expr;
}

/// Determines the token type according to the table key and generates a symbolic variable for it.
const IR::Expression *makeConstant(Token input, const IdenitifierTypeMap &typeMap,
                                   const IR::Type *leftType) {
    const IR::Type_Base *type = nullptr;
    const IR::Expression *result = nullptr;
    auto inputStr = input.lexeme();
    if (input.is(Token::Kind::Text)) {
        for (const auto &[identifier, keyType] : typeMap) {
            auto annoSize = identifier.size();
            auto tokenLength = inputStr.length();
            if (inputStr.compare(tokenLength - annoSize, annoSize, identifier) != 0) {
                continue;
            }
            if (const auto *bit = keyType->to<IR::Type_Bits>()) {
                type = bit;
            } else if (const auto *varbit = keyType->to<IR::Extracted_Varbits>()) {
                type = varbit;
            } else if (keyType->is<IR::Type_Boolean>()) {
                type = IR::Type_Bits::get(1);
            } else {
                BUG("Unexpected key type %s.", keyType->node_type_name());
            }
            return ToolsVariables::getSymbolicVariable(type, std::string(inputStr));
        }
    }
    if (input.is(Token::Kind::Number)) {
        if (leftType == nullptr) {
            return IR::getConstant(IR::Type_Bits::get(32), big_int(std::string(inputStr)));
        }
        return IR::getConstant(leftType, big_int(std::string(inputStr)));
    }
    // TODO: Is this the right solution for priorities?
    if (input.is(Token::Kind::Priority)) {
        return ToolsVariables::getSymbolicVariable(IR::Type_Bits::get(32), std::string(inputStr));
    }
    BUG_CHECK(result != nullptr,
              "Could not match restriction key label %s was not found in key list.",
              std::string(inputStr));
    return result;
}

/// Determines the right side of the expression starting from the original position and returns a
/// slice of tokens related to the right side and the index of the end of the right side.
/// Returning the end index is necessary so that after moving from the end of the right side
std::pair<std::vector<Token>, size_t> findRightPart(std::vector<Token> tokens, size_t index) {
    size_t idx = index + 1;
    size_t endIdx = 0;
    bool flag = true;
    while (flag) {
        if (idx == tokens.size() ||
            tokens[idx].isOneOf(Token::Kind::Conjunction, Token::Kind::Disjunction,
                                Token::Kind::RightParen)) {
            endIdx = idx;
            flag = false;
        }
        idx++;
    }

    std::vector<Token> rightTokens;
    for (size_t j = index + 1; j < endIdx; j++) {
        rightTokens.push_back(tokens[j]);
    }
    return std::make_pair(rightTokens, idx);
}

/// Chose a binary expression that correlates to the token kind.
const IR::Expression *pickBinaryExpr(const Token &token, const IR::Expression *leftL,
                                     const IR::Expression *rightL) {
    if (token.is(Token::Kind::Minus)) {
        return new IR::Sub(leftL, rightL);
    }
    if (token.is(Token::Kind::Plus)) {
        return new IR::Add(leftL, rightL);
    }
    if (token.is(Token::Kind::Equal)) {
        return new IR::Equ(leftL, rightL);
    }
    if (token.is(Token::Kind::NotEqual)) {
        return new IR::Neq(leftL, rightL);
    }
    if (token.is(Token::Kind::GreaterThan)) {
        return new IR::Grt(leftL, rightL);
    }
    if (token.is(Token::Kind::GreaterEqual)) {
        return new IR::Geq(leftL, rightL);
    }
    if (token.is(Token::Kind::LessThan)) {
        return new IR::Lss(leftL, rightL);
    }
    if (token.is(Token::Kind::LessEqual)) {
        return new IR::Leq(leftL, rightL);
    }
    if (token.is(Token::Kind::Slash)) {
        return new IR::Div(leftL, rightL);
    }
    if (token.is(Token::Kind::Percent)) {
        return new IR::Mod(leftL, rightL);
    }
    if (token.is(Token::Kind::Shr)) {
        return new IR::Shr(leftL, rightL);
    }
    if (token.is(Token::Kind::Shl)) {
        return new IR::Shl(leftL, rightL);
    }
    if (token.is(Token::Kind::Mul)) {
        return new IR::Mul(leftL, rightL);
    }
    if (token.is(Token::Kind::NotEqual)) {
        return new IR::Neq(leftL, rightL);
    }
    BUG("Unsupported binary expression.");
}

/// Converts a vector of tokens into a single IR:Expression
/// For example, at the input we have a vector of tokens:
/// [key1(Text), ->(Implication), key2(Text), &&(Conjunction), key3(Text)] The result will be an
/// IR::Expression equal to !IR::Expression || (IR::Expression && IR::Expression)
const IR::Expression *getIR(std::vector<Token> tokens, const IdenitifierTypeMap &typeMap) {
    std::vector<const IR::Expression *> exprVec;

    for (size_t idx = 0; idx < tokens.size(); idx++) {
        auto token = tokens.at(idx);
        if (token.isOneOf(Token::Kind::Minus, Token::Kind::Plus, Token::Kind::Equal,
                          Token::Kind::NotEqual, Token::Kind::GreaterThan,
                          Token::Kind::GreaterEqual, Token::Kind::LessThan, Token::Kind::LessEqual,
                          Token::Kind::Slash, Token::Kind::Percent, Token::Kind::Shr,
                          Token::Kind::Shl, Token::Kind::Mul, Token::Kind::NotEqual)) {
            const IR::Expression *leftL = nullptr;
            const IR::Expression *rightL = nullptr;
            leftL = makeConstant(tokens[idx - 1], typeMap, nullptr);
            if (tokens[idx + 1].isOneOf(Token::Kind::Text, Token::Kind::Number)) {
                rightL = makeConstant(tokens[idx + 1], typeMap, leftL->type);
                if (const auto *constant = leftL->to<IR::Constant>()) {
                    auto *clone = constant->clone();
                    clone->type = rightL->type;
                    leftL = clone;
                }
            } else {
                auto rightPart = findRightPart(tokens, idx);
                rightL = getIR(rightPart.first, typeMap);
                idx = rightPart.second;
            }

            if (idx >= 2 && tokens[idx - 2].is(Token::Kind::LNot)) {
                leftL = new IR::LNot(leftL);
            }
            exprVec.push_back(pickBinaryExpr(token, leftL, rightL));
        } else if (token.is(Token::Kind::LNot)) {
            if (!tokens[idx + 1].isOneOf(Token::Kind::Text, Token::Kind::Number)) {
                auto rightPart = findRightPart(tokens, idx);
                const IR::Expression *exprLNot = getIR(rightPart.first, typeMap);
                idx = rightPart.second;
                exprVec.push_back(new IR::LNot(exprLNot));
            }
        } else if (token.isOneOf(Token::Kind::Disjunction, Token::Kind::Implication)) {
            if (token.is(Token::Kind::Implication)) {
                const auto *tmp = exprVec[exprVec.size() - 1];
                exprVec.pop_back();
                exprVec.push_back(new IR::LNot(tmp));
            }
            const IR::Expression *expr1 = new IR::PathExpression(new IR::Path("tmp1"));
            const IR::Expression *expr2 = new IR::PathExpression(new IR::Path("tmp2"));
            exprVec.push_back(new IR::LOr(expr1, expr2));
        } else if (token.is(Token::Kind::Conjunction)) {
            const IR::Expression *expr1 = new IR::PathExpression(new IR::Path("tmp1"));
            const IR::Expression *expr2 = new IR::PathExpression(new IR::Path("tmp2"));
            exprVec.push_back(new IR::LAnd(expr1, expr2));
        }
    }

    if (exprVec.size() == 1) {
        return exprVec[0];
    }

    return makeSingleExpr(exprVec);
}
/// Combines successive tokens into variable names.
/// Returns a vector with tokens combined into names.
/// For example, at the input we have a vector of tokens:
/// [a(Text),c(Text),b(text), +(Plus), 1(Number),2(number)]
/// The result will be [acb(Text), +(Plus), 1(Number),2(number)]
std::vector<Token> combineTokensToNames(const std::vector<Token> &inputVector) {
    std::vector<Token> result;
    Token prevToken = Token(Token::Kind::Unknown, " ", 1);
    cstring txt = "";
    for (const auto &input : inputVector) {
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

/// Combines successive tokens into numbers. For converting boolean or Hex, Octal values to
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
        if (input[i].is(Token::Kind::Minus)) {
            numb += "-";
            i++;
        }

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
/// Convert access tokens or keys into table keys. For converting access tokens or keys to
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
    for (uint64_t idx = 0; idx < input.size(); idx++) {
        if (!input[idx].is(Token::Kind::Text)) {
            result.push_back(input[idx]);
            continue;
        }
        auto str = std::string(input[idx].lexeme());

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
            cstring cstr = tableName + "_key_" + str;
            result.emplace_back(Token::Kind::Text, cstr, cstr.size());
            idx += 2;
            continue;
        }

        substr = str.substr(0, str.find("::value"));
        if (substr != str) {
            cstring cstr = tableName + "_key_" + substr;
            result.emplace_back(Token::Kind::Text, cstr, cstr.size());
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
std::vector<Token> removeComments(const std::vector<Token> &input) {
    std::vector<Token> result;
    bool flag = true;
    for (const auto &i : input) {
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
std::vector<const IR::Expression *> AssertsParser::genIRStructs(cstring tableName,
                                                                cstring restrictionString,
                                                                const IdenitifierTypeMap &typeMap) {
    Lexer lex(restrictionString);
    std::vector<Token> tmp;
    for (auto token = lex.next(); !token.isOneOf(Token::Kind::End, Token::Kind::Unknown);
         token = lex.next()) {
        tmp.push_back(token);
    }

    std::vector<const IR::Expression *> result;

    tmp = combineTokensToNames(tmp);
    tmp = combineTokensToNumbers(tmp);
    tmp = combineTokensToTableKeys(tmp, tableName);
    tmp = removeComments(tmp);
    std::vector<Token> tokens;
    for (uint64_t i = 0; i < tmp.size(); i++) {
        if (tmp[i].is(Token::Kind::Semicolon)) {
            const auto *expr = getIR(tokens, typeMap);
            result.push_back(expr);
            tokens.clear();
        } else if (i == tmp.size() - 1) {
            tokens.push_back(tmp[i]);
            const auto *expr = getIR(tokens, typeMap);
            result.push_back(expr);
            tokens.clear();
        } else {
            tokens.push_back(tmp[i]);
        }
    }

    return result;
}

const IR::Node *AssertsParser::postorder(IR::P4Action *actionContext) {
    const auto *annotation = actionContext->getAnnotation("action_restriction");
    if (annotation == nullptr) {
        return actionContext;
    }

    IdenitifierTypeMap typeMap;
    for (const auto *arg : actionContext->parameters->parameters) {
        typeMap[arg->controlPlaneName()] = arg->type;
    }

    for (const auto *restrStr : annotation->body) {
        auto restrictions =
            genIRStructs(actionContext->controlPlaneName(), restrStr->text, typeMap);
        // Using Z3Solver, we check the feasibility of restrictions, if they are not
        // feasible, we delete keys and entries from the table to execute
        // default_action
        restrictionsVec.insert(restrictionsVec.begin(), restrictions.begin(), restrictions.end());
    }
    return actionContext;
}

const IR::Node *AssertsParser::postorder(IR::P4Table *tableContext) {
    const auto *annotation = tableContext->getAnnotation("entry_restriction");
    const auto *key = tableContext->getKey();
    if (annotation == nullptr || key == nullptr) {
        return tableContext;
    }

    IdenitifierTypeMap typeMap;
    for (const auto *keyElement : tableContext->getKey()->keyElements) {
        const auto *nameAnnot = keyElement->getAnnotation("name");
        BUG_CHECK(nameAnnot != nullptr, "%1% table key without a name annotation",
                  annotation->name.name);
        typeMap[nameAnnot->getName()] = keyElement->expression->type;
    }

    Z3Solver solver;
    for (const auto *restrStr : annotation->body) {
        auto restrictions = genIRStructs(tableContext->controlPlaneName(), restrStr->text, typeMap);
        // Using Z3Solver, we check the feasibility of restrictions, if they are not
        // feasible, we delete keys and entries from the table to execute
        // default_action
        solver.push();
        if (solver.checkSat(restrictions) == true) {
            restrictionsVec.insert(restrictionsVec.begin(), restrictions.begin(),
                                   restrictions.end());
            continue;
        }
        ::warning(
            "Restriction %1% is not feasible. Not generating entries for table %2% and instead "
            "using default action.",
            restrStr, tableContext);
        solver.pop();
        auto *cloneTable = tableContext->clone();
        auto *cloneProperties = tableContext->properties->clone();
        IR::IndexedVector<IR::Property> properties;
        for (const auto *property : cloneProperties->properties) {
            if (property->name.name != "key" && property->name.name != "entries") {
                properties.push_back(property);
            }
        }
        cloneProperties->properties = properties;
        cloneTable->properties = cloneProperties;
        return cloneTable;
    }
    return tableContext;
}

Token Lexer::atom(Token::Kind kind) noexcept { return {kind, mBeg++, 1}; }

Token Lexer::next() noexcept {
    while (isSpace(peek())) {
        get();
    }

    switch (peek()) {
        case '\0':
            return {Token::Kind::End, mBeg, 1};
        case '\n':
            get();
            return {Token::Kind::EndString, mBeg, 1};
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
}  // namespace P4Tools::P4Testgen::Bmv2
