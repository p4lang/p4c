#include "backends/p4tools/common/compiler/p4_expr_parser.h"

namespace P4Tools {
namespace ExpressionParser {

static std::vector<std::string> NAMES {
    "Priority",      "Text",        "LineStatementClose", "Number",        "Comment", 
    "StringLiteral", "LeftParen",   "RightParen",         "LeftSParen",   "RightSParen",
    "Dot",           "FieldAccess", "LNot",               "Complement",    "Mul",
    "Percent",       "Slash",       "Minus",              "SaturationSub", "Plus",
    "SaturationAdd", "LessEqual",   "Shl",                "LessThan",      "GreaterEqual",
    "Shr",           "GreaterThan", "NotEqual",           "Equal",         "BAnd",
    "Xor",           "BOr",         "Conjunction",        "Disjunction",   "Implication",
    "Colon",         "Question",    "Semicolon",          "Comma",         "Unknown",
    "EndString",     "End"
};


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

/// Get string for lexeme
std::string Token::strLexeme() const noexcept {
    return {m_lexeme.data(), m_lexeme.size()};
}

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
    return os << NAMES.at(static_cast<size_t>(kind));
}
template<class T>
NodesPair turnLeft(const IR::Node* left, const IR::Node* right) {
    if (right == nullptr) {
        return NodesPair(left, right);
    }
    if (const auto* expr = right->to<T>()) {
        left = new T(left->to<IR::Expression>(), expr->left);
        right = expr->right;
        return turnLeft<T>(left, right);
    }
    return NodesPair(left, right);
}

NodesPair Parser::makeLeftTree(Token::Kind kind, const IR::Node* left, const IR::Node* right) {
    switch (kind) {
        case Token::Kind::Percent:
            return turnLeft<IR::Mod>(left, right);
        case Token::Kind::Slash:
            return turnLeft<IR::Div>(left, right);
        case Token::Kind::Minus:
            return turnLeft<IR::Sub>(left, right);
        case Token::Kind::SaturationSub:
            return turnLeft<IR::SubSat>(left, right);
        case Token::Kind::Shl:
            return turnLeft<IR::Shl>(left, right);
        case Token::Kind::Shr:
            return turnLeft<IR::Shr>(left, right);
        default:
            return NodesPair(left, right);
    };
}

const IR::Node* Parser::removeBrackets(const IR::Node* expr) {
    if (const auto* namedExpr = expr->to<IR::NamedExpression>()) {
        const auto* listExpr = namedExpr->to<IR::ListExpression>();
        BUG_CHECK(namedExpr->name == "Paren", "Invalid format %1%", namedExpr);
        BUG_CHECK(listExpr != nullptr, "Unsupported format for named expression %1%", expr);
        BUG_CHECK(listExpr->size() == 1, "");
        return listExpr->components.at(0);
    }
    return expr;
}

const IR::Node* Parser::createIR(Token::Kind kind, const IR::Node* left,
                                 const IR::Node* right) {
    auto res = makeLeftTree(kind, left, right);
    left = removeBrackets(res.first);
    right = removeBrackets(res.second);
    const auto* leftExpr = left->to<IR::Expression>();
    const auto* rightExpr = right->to<IR::Expression>();
    switch (kind) {
        case Token::Kind::LNot:
            return new IR::LNot(leftExpr);
        case Token::Kind::Complement:
            return new IR::Cmpl(leftExpr);
        case Token::Kind::Mul:
            return new IR::Mul(leftExpr, rightExpr);
        case Token::Kind::Percent:
            return new IR::Mod(leftExpr, rightExpr);
        case Token::Kind::Slash:
            return new IR::Div(leftExpr, rightExpr);
        case Token::Kind::Minus:
            return new IR::Sub(leftExpr, rightExpr);
        case Token::Kind::SaturationSub:
            return new IR::SubSat(leftExpr, rightExpr);
        case Token::Kind::Plus:
            return new IR::Add(leftExpr, rightExpr);
        case Token::Kind::SaturationAdd:
            return new IR::AddSat(leftExpr, rightExpr);
        case Token::Kind::LessEqual:
            return new IR::Leq(leftExpr, rightExpr);
        case Token::Kind::Shl:
            return new IR::Shl(leftExpr, rightExpr);
        case Token::Kind::LessThan:
            return new IR::Lss(leftExpr, rightExpr);
        case Token::Kind::GreaterEqual:
            return new IR::Geq(leftExpr, rightExpr);
        case Token::Kind::Shr:
            return new IR::Shr(leftExpr, rightExpr);
        case Token::Kind::GreaterThan:
            return new IR::Grt(leftExpr, rightExpr);
        case Token::Kind::NotEqual:
            return new IR::Neq(leftExpr, rightExpr);
        case Token::Kind::Equal:
            return new IR::Equ(leftExpr, rightExpr);
        case Token::Kind::BAnd:
            return new IR::BAnd(leftExpr, rightExpr);
        case Token::Kind::Xor:
            return new IR::BXor(leftExpr, rightExpr);
        case Token::Kind::BOr:
            return new IR::BOr(leftExpr, rightExpr);
        case Token::Kind::Conjunction:
            return new IR::LAnd(leftExpr, rightExpr);
        case Token::Kind::Disjunction:
            return new IR::LOr(leftExpr, rightExpr);
        case Token::Kind::Implication:
            return new IR::LOr(new IR::LNot(leftExpr), rightExpr);
        default:
            BUG("Unimplemented kind %1%", kind);
    }
}

std::pair<const IR::Node*, IR::ID> Parser::getDefinedType(cstring txt, const IR::Node* nd) {
    if (nd == nullptr) {
        return getDefinedType(txt, program);
    } else if (const auto* typeName = nd->to<IR::Type_Name>()) {
        return getDefinedType(txt, getDefinedType(typeName->path->name, nullptr).first);
    } else {
        const IR::IDeclaration *decl = nullptr;
        if (nd->is<IR::P4Program>()) {
            auto* decls = program->getDeclsByName(txt);
            BUG_CHECK(decls != nullptr, "Can't find type for %1%", txt);
            decl = decls->single();
        } else if (const auto* ns = nd->to<IR::ISimpleNamespace>()) {
            decl = ns->getDeclByName(txt);
            if (decl == nullptr) {
                if (const auto* control = nd->to<IR::P4Control>()) {
                    for (const auto* d : control->controlLocals) {
                        if (d->name.originalName == txt) {
                            if (const auto* var = d->to<IR::Declaration_Variable>()){
                                return std::make_pair(var->type, d->name);
                            }
                            if (const auto* c = d->to<IR::Declaration_Constant>()) {
                                return std::make_pair(c->type, d->name);
                            }
                            if (const auto* i = d->to<IR::Declaration_Instance>()) {
                                return std::make_pair(i->type, d->name);
                            }
                        }
                    }
                }
            } else if (const auto* field = decl->to<IR::StructField>()) {
                return std::make_pair(field->type, field->name);
            }
            if (decl == nullptr) {
                // Find in parameters
                if (const auto iapply = nd->to<IR::IApply>()) {
                    const auto paramList = iapply->getApplyParameters();
                    decl = paramList->getDeclByName(txt);
                    if (decl != nullptr && decl->is<IR::Parameter>()) {
                        const auto* param = decl->to<IR::Parameter>();
                        return std::make_pair(param->type, param->name);
                    }
                }
            }
        }
        if (decl == nullptr) {
            if (const auto* strct = nd->to<IR::Type_StructLike>()) {
                const auto *field = strct->getField(txt);
                if (field == nullptr) {
                    for (auto i : strct->fields) {
                        if (i->name.originalName == txt) {
                            return std::make_pair(i->type, i->name);
                        }
                    }
                } else {
                    return std::make_pair(field->type, IR::ID(txt));
                }
            }
            BUG_CHECK(!nd->is<IR::P4Control>() && !nd->is<IR::P4Action>() && !nd->is<IR::P4Table>(), "Can't find %1% in %2%", txt, nd);
            // Retrun Boolean type by default.
            return std::make_pair(IR::Type_Boolean::get(), IR::ID(txt));
        }
        return std::make_pair(decl->to<IR::Node>(), IR::ID(txt));
    }
}

bool isValidNumber(Token t, bool isHex) {
    if (tokens[index].is(Token::Kind::Number)) {
        return true;
    }
    if (!isHex) {
        return false;
    }
    cstring s = t.strLexeme)();
    return (s == 'a' || s == 'A' || s == 'b' || s == 'B' || s == 'c' || s == 'C' ||
            s == 'd' || s == 'D' || s == 'e' || s == 'E' || s == 'f' || s == 'F');
}

const IR::Node* Parser::createConstantOp() {
    LOG1("createConstantOp : " << tokens[index].lexeme());
    if (tokens[index].is(Token::Kind::Text)) {
        cstring txt = "";
        do {
            txt += tokens[index].strLexeme();
            index++;
        } while (tokens[index].is(Token::Kind::Text) || tokens[index].is(Token::Kind::Number));
        if (txt == "true") {
            return new IR::BoolLiteral(true);
        } else if (txt == "false") {
            return new IR::BoolLiteral(false);
        } else {
            return new IR::NamedExpression("FieldName", new IR::StringLiteral(txt));
        }
    }
    if (tokens[index].is(Token::Kind::Number)) {
        std::string txt;
        bool isHex = false;
        bool wasX = fasle;
        do {
            txt += std::data(tokens[index].lexeme());
            index++;
            if (tokens[index].is(Token::Kind::Text)) {
                if (tokens[index].lexeme() == "x" && index + 1 < tokens.size() &&
                    isValidNumber(tokens[index + 1], isHex)) {
                    txt.erase(0, 1);
                    isHex = true;
                    index++;
                    continue;
                }
                break;
            }
        } while (isValidNumber(tokens[index + 1], isHex));
        const IR::Expression* expression = nullptr;
        if (isHex) {
            std::istringstream converter { txt };
            uint64_t value = 0;
            converter >> std::hex >> value;
            expression = new IR::Constant(value);
        } else {
            expression = new IR::Constant(big_int(txt));
        }
        return expression;
    }
    BUG("Unimplemented literal %1%", tokens[index].lexeme());
}

const IR::Node* Parser::createSliceOrArrayOp(const IR::Node* base) {
    LOG1("createSliceOp : " << tokens[index].lexeme());
    BUG_CHECK(tokens[index].is(Token::Kind::LeftSParen), "Expected '['");
    const auto slice = createFunctionCallOrConstantOp();
    BUG_CHECK(tokens[index].is(Token::Kind::RightSParen), "Expected ']'");
    index++;
    const auto* namedExpr = slice->to<IR::NamedExpression>();
    BUG_CHECK(namedExpr != nullptr, "Unexpected expression %1%", namedExpr);
    BUG_CHECK(namedExpr->name == "Colon", "Expected colon expression %1%", namedExpr);
    const auto* listExpr = namedExpr->expression->to<IR::ListExpression>();
    BUG_CHECK(listExpr->size() < 2 && listExpr->size() != 0, "Expected one or two arguments", listExpr);
    if (listExpr->size() == 1) {
        // Create an array.
        return new IR::ArrayIndex(base->to<IR::Expression>(), listExpr->components.at(0));
    }
    // Create a slice.
    return new IR::Slice(base->to<IR::Expression>(), listExpr->components.at(0), listExpr->components.at(1));
}

const IR::Node* Parser::createApplicationOp(const IR::Node* base) {
    if (!tokens[index].is(Token::Kind::LeftParen)) {
        return base;
    }
    const auto* params = createFunctionCallOrConstantOp();
    IR::Vector<IR::Argument>* arguments = new IR::Vector<IR::Argument>();
    const auto* namedExpr = params->to<IR::NamedExpression>();
    BUG_CHECK(namedExpr != nullptr, "Invalid format %1%", namedExpr);
    BUG_CHECK(namedExpr->name == "Paren", "Invalid format %1%", params);
    const auto* listExpr = namedExpr->expression->to<IR::ListExpression>();
    BUG_CHECK(listExpr != nullptr, "Invalid format %1%", namedExpr->expression);
    for (const auto* p : listExpr->components) {
        arguments->push_back(new IR::Argument(p));
    }
    return new IR::MethodCallExpression(base->to<IR::Expression>(), arguments);
}

const IR::Type* Parser::ndToType(const IR::Node* nd) {
    if (nd->is<IR::ISimpleNamespace>()) {
        if (const auto* control = nd->to<IR::P4Control>()) {
            return control->type;
        }
        if (const auto* action = nd->to<IR::P4Action>()) {
            return new IR::Type_Action(action->srcInfo, action->parameters);
        }
        if (const auto* table = nd->to<IR::P4Table>()) {
            return new IR::Type_Table(table);
        }
    }
    if (const auto* typeName = nd->to<IR::Type_Name>()) {
        return ndToType(getDefinedType(typeName->path->name, nullptr).first);
    }
    return nd->to<IR::Type>();
}

const IR::Node* Parser::createFunctionCallOrConstantOp() {
    LOG1("createFunctionCallOrConstantOp : " << tokens[index].lexeme());
    if (tokens[index].is(Token::Kind::Minus)) {
        index++;
        return new IR::Mul(new IR::Constant(-1), createFunctionCallOrConstantOp()->to<IR::Expression>());
    }
    if (tokens[index].is(Token::Kind::Plus)) {
        index++;
        return createFunctionCallOrConstantOp();
    }
    if (tokens[index].is(Token::Kind::StringLiteral)) {
        index++;
        std::string txt = tokens[index].strLexeme();
        return new IR::StringLiteral(txt);
    }
    if (tokens[index].is(Token::Kind::LeftParen)) {
        index++;
        if (tokens[index].is(Token::Kind::RightParen)) {
            index++;
            IR::Vector<IR::Expression> components;
            return new IR::NamedExpression("Paren", new IR::ListExpression(components));
        }
        const auto* res = createPunctuationMarks();
        BUG_CHECK(tokens[index].is(Token::Kind::RightParen), "Can't find coresponded ']'");
        index++;
        return new IR::NamedExpression("Paren", res->to<IR::Expression>());
    }
    const auto* mainArgument = createConstantOp();
    if (index >= tokens.size()) {
        return mainArgument;
    }
    const IR::Node* nd = nullptr;
    if (const auto* namedExpr = mainArgument->to<IR::NamedExpression>()) {
        if (namedExpr->name == "FieldName") {
            const auto name = namedExpr->expression->to<IR::StringLiteral>()->value;
            auto res = getDefinedType(name, nullptr);
            nd = res.first;
            mainArgument = new IR::PathExpression(ndToType(res.first), new IR::Path(res.second));
        }
    }
    LOG1("createFunctionCallOrConstantOp next : " << tokens[index-1].lexeme());
    LOG1("createFunctionCallOrConstantOp next : " << tokens[index].lexeme());
    LOG1("createFunctionCallOrConstantOp next : " << tokens[index+1].lexeme());
    if (tokens[index].is(Token::Kind::Dot) || tokens[index].is(Token::Kind::FieldAccess)) {
        if (nd == nullptr) {
            nd = getDefinedType(mainArgument->to<IR::NamedExpression>()->expression->to<IR::StringLiteral>()->value, nullptr).first;
        }
        do {
            index++;
            auto result = createConstantOp();
            cstring name = result->to<IR::NamedExpression>()->expression->to<IR::StringLiteral>()->value;
            auto res = getDefinedType(name, nd);
            nd = res.first;
            mainArgument =  new IR::Member(ndToType(res.first), mainArgument->to<IR::Expression>(), res.second);
        } while (tokens[index].is(Token::Kind::Dot) || tokens[index].is(Token::Kind::FieldAccess));
        if (index >= tokens.size()) {
            return mainArgument;
        }
        if (tokens[index].is(Token::Kind::LeftSParen)) {
            return createSliceOrArrayOp(mainArgument);
        }
        if (tokens[index].is(Token::Kind::LeftParen)) {
            return createApplicationOp(mainArgument);
        }
        return mainArgument;
    }
    if (tokens[index].is(Token::Kind::LeftSParen) && mainArgument->is<IR::PathExpression>()) {
        return createSliceOrArrayOp(mainArgument);
    }
    if (tokens[index].is(Token::Kind::LeftParen)) {
            return createApplicationOp(mainArgument);
        }
    return mainArgument;
}

const IR::Node* Parser::createArithmeticOp() {
    LOG1("createArithmeticOp : " << tokens[index].lexeme());
    auto curToken = tokens[index];
    if (curToken.is(Token::Kind::LNot)) {
        index++;
        return createIR(curToken.kind(), createLogicalOp(), nullptr);
    }
    if (curToken.is(Token::Kind::Complement)) {
        index++;
        return createIR(curToken.kind(), createLogicalOp(), nullptr);
    }
    const auto* mainArgument = createFunctionCallOrConstantOp();
    if (index >= tokens.size()) {
        return mainArgument;
    }
    LOG1("createArithmeticOp next : " << tokens[index].lexeme());
    index++;
    if (curToken.is(Token::Kind::Mul)) {
        return createIR(curToken.kind(), mainArgument, createArithmeticOp());
    }
    if (curToken.is(Token::Kind::Percent)) {
        return createIR(curToken.kind(), mainArgument, createArithmeticOp());
    }
    if (curToken.is(Token::Kind::Slash)) {
        return createIR(curToken.kind(), mainArgument, createArithmeticOp());
    }
    if (curToken.is(Token::Kind::Minus)) {
        return createIR(curToken.kind(), mainArgument, createArithmeticOp());
    }
    if (curToken.is(Token::Kind::SaturationSub)) {
        return createIR(curToken.kind(), mainArgument, createArithmeticOp());
    }
    if (curToken.is(Token::Kind::Plus)) {
        return createIR(curToken.kind(), mainArgument, createArithmeticOp());
    }
    if (curToken.is(Token::Kind::SaturationAdd)) {
        return createIR(curToken.kind(), mainArgument, createArithmeticOp());
    }
    index--;
    return mainArgument;
}

const IR::Node* Parser::createEqCompareAndShiftOp() {
    LOG1("createEqCompareAndShiftOp : " << tokens[index].lexeme());
    const auto* mainArgument = createArithmeticOp();
    if (index >= tokens.size()) {
        return mainArgument;
    }
    LOG1("createEqCompareAndShiftOp : " << tokens[index].lexeme());
    auto curToken = tokens[index];
    index++;
    if (curToken.is(Token::Kind::LessEqual)) {
        return createIR(curToken.kind(), mainArgument, createEqCompareAndShiftOp());
    }
    if (curToken.is(Token::Kind::Shl)) {
        return createIR(curToken.kind(), mainArgument, createEqCompareAndShiftOp());
    }
    if (curToken.is(Token::Kind::LessThan)) {
        return createIR(curToken.kind(), mainArgument, createEqCompareAndShiftOp());
    }
    if (curToken.is(Token::Kind::GreaterEqual)) {
        return createIR(curToken.kind(), mainArgument, createEqCompareAndShiftOp());
    }
    if (curToken.is(Token::Kind::Shr)) {
        return createIR(curToken.kind(), mainArgument, createEqCompareAndShiftOp());
    }
    if (curToken.is(Token::Kind::GreaterThan)) {
        return createIR(curToken.kind(), mainArgument, createEqCompareAndShiftOp());
    }
    if (curToken.is(Token::Kind::NotEqual)) {
        return createIR(curToken.kind(), mainArgument, createEqCompareAndShiftOp());
    }
    if (curToken.is(Token::Kind::Equal)) {
        return createIR(curToken.kind(), mainArgument, createEqCompareAndShiftOp());
    }
    index--;
    return mainArgument;
}

const IR::Node* Parser::createBinaryOp() {
    LOG1("createBinaryOp : " << tokens[index].lexeme());
    const auto* mainArgument = createEqCompareAndShiftOp();
    if (index >= tokens.size()) {
        return mainArgument;
    }
    LOG1("createBinaryOp next : " << tokens[index].lexeme());
    auto curToken = tokens[index];
    index++;
    if (curToken.is(Token::Kind::BAnd)) {
        return createIR(curToken.kind(), mainArgument, createBinaryOp());
    }
    if (curToken.is(Token::Kind::Xor)) {
        return createIR(curToken.kind(), mainArgument, createBinaryOp());
    }
    if (curToken.is(Token::Kind::BOr)) {
        return createIR(curToken.kind(), mainArgument, createBinaryOp());
    }
    index--;
    return mainArgument;
}

const IR::Node* Parser::createLogicalOp() {
    LOG1("createLogicalOp : " << tokens[index].lexeme());
    BUG_CHECK(index < tokens.size(), "Invalid index of a token in createLogicalOp");
    const auto* mainArgument = createBinaryOp();
    if (index >= tokens.size()) {
        return mainArgument;
    }
    LOG1("createLogicalOp next : " << tokens[index].lexeme());
    auto curToken = tokens[index];
    index++;
    if (curToken.is(Token::Kind::Conjunction)) {
        return createIR(curToken.kind(), mainArgument, createLogicalOp());
    }
    if (curToken.is(Token::Kind::Disjunction)) {
        return createIR(curToken.kind(), mainArgument, createLogicalOp());
    }
    if (curToken.is(Token::Kind::Implication)) {
        return createIR(curToken.kind(), mainArgument, createLogicalOp());
    }
    index--;
    return mainArgument;
}

const IR::Node* Parser::createListExpressions(const IR::Node* first, const char* name, Token::Kind kind) {
    IR::Vector<IR::Expression> components;
    components.push_back(first->to<IR::Expression>());
    do {
        index++;
        components.push_back(createPunctuationMarks()->to<IR::Expression>());
    } while (tokens[index].kind() == kind);
    return new IR::NamedExpression(name, new IR::ListExpression(components));
}

const IR::Node* Parser::createPunctuationMarks() {
    LOG1("createPunctuationMarks : " << tokens[index].lexeme());
    const auto* mainArgument = createLogicalOp();
    if (index >= tokens.size()) {
        return mainArgument;
    }
    LOG1("createPunctuationMarks next : " << tokens[index].lexeme());
    if (tokens[index].is(Token::Kind::Colon)) {
        return createListExpressions(mainArgument, NAMES[static_cast<size_t>(tokens[index].kind())].c_str(), tokens[index].kind());
    }
    if (tokens[index].is(Token::Kind::Question)) {
        index++;
        const auto* right = createPunctuationMarks()->to<IR::Expression>();
        const auto* namedExpr = right->to<IR::NamedExpression>();
        BUG_CHECK(namedExpr->name == "Colon", "Undefined named expression %1%", right);
        const auto* listExpr = namedExpr->expression->to<IR::ListExpression>();
        BUG_CHECK(listExpr->size() == 2,
                  "Invalid size for arguments of ? operator %1%", right);
        return new IR::Mux(mainArgument->to<IR::Expression>(), listExpr->components[0], listExpr->components[0]);
    }
    if (tokens[index].is(Token::Kind::Semicolon)) {
        return createListExpressions(mainArgument, NAMES[static_cast<size_t>(tokens[index].kind())].c_str(),
                                     tokens[index].kind());
    }
    if (tokens[index].is(Token::Kind::Comma)) {
        return createListExpressions(mainArgument, NAMES[static_cast<size_t>(tokens[index].kind())].c_str(),
                                     tokens[index].kind());
    }
    return mainArgument;
}

const IR::Node* Parser::getIR() {
    index = 0;
    LOG1("getIR : " << tokens[index].lexeme());
    BUG_CHECK(index < tokens.size(), "Invalid size of the tokens vector");
    const auto* result = createPunctuationMarks();
    BUG_CHECK(index >= tokens.size(), "Can't translate string into IR");
    std::cout << result << std::endl;
    return result;
}

const IR::Node* Parser::getIR(const char* str, const IR::P4Program* program,
                              TokensSet skippedTokens) {
    Lexer lex(str);
    std::vector<Token> tmp;
    for (auto token = lex.next(); !token.is_one_of(Token::Kind::End, Token::Kind::Unknown);
         token = lex.next()) {
        if (skippedTokens.count(token.kind()) == 0) {
            tmp.push_back(token);
        }
    }
    Parser parser(program, tmp);
    return parser.getIR();
}

Parser::Parser(const IR::P4Program* program, std::vector<Token> &tokens)
    : program(program), tokens(tokens) {
}

Token Lexer::atom(Token::Kind kind) noexcept { return {kind, m_beg++, 1}; }

Token Lexer::next() noexcept {
    while (isSpace(peek())) {
        get();
    }
    std::string txt;
    bool isAdd = false;
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
        case '\"':
            txt = "";
            do {
                txt += peek();
            } while (get() != '\"');
            return {Token::Kind::StringLiteral, txt.c_str(), txt.length()};
        case '(':
            return atom(Token::Kind::LeftParen);
        case ')':
            return atom(Token::Kind::RightParen);
        case '=':
            get();
            if (get() == '=') {
                return {Token::Kind::Equal, "==", 2};
            }
            prev();
            prev();
            return atom(Token::Kind::Text);
        case '!':
            get();
            if (get() == '=') {
                return {Token::Kind::NotEqual, "!=", 2};
            }
            prev();
            return atom(Token::Kind::LNot);
        case '-':
            get();
            if (get() == '>') {
                return {Token::Kind::Implication, "->", 2};
            }
            prev();
            return atom(Token::Kind::Minus);
        case '<':
            get();
            if (get() == '=') {
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
        case ',':
            return atom(Token::Kind::Comma);
        case '.':
            return atom(Token::Kind::Dot);
        case ':':
            get();
            if (get() == ':') {
                return {Token::Kind::FieldAccess, "::", 2};
            }
            prev();
            return {Token::Kind::FieldAccess, ":", 1};
        case '&':
            get();
            if (get() == '&') {
                return {Token::Kind::Conjunction, "&&", 2};
            }
            prev();
            prev();
            return atom(Token::Kind::Text);
        case '|':
            get();
            if (get() == '|') {
                return {Token::Kind::Disjunction, "||", 2};
            }
            prev();
            if ((isAdd = (peek() == '+')) || peek() == '-') {
                get();
                if (peek() == '|') {
                    get();
                    if (isAdd) {
                        return atom(Token::Kind::SaturationAdd);
                    } else {
                        return atom(Token::Kind::SaturationSub);
                    }
                }
                prev();
            }
            prev();
            return atom(Token::Kind::Text);
        case '+':
            return atom(Token::Kind::Plus);
        case '/':
            get();
            if (get() == '/') {
                txt = "";
                while (peek() != '\n') {
                    txt += get();
                }
                return {Token::Kind::Comment, txt.c_str(), txt.length()};
            }
            prev();
            prev();
            return atom(Token::Kind::Slash);
        case '%':
            return atom(Token::Kind::Percent);
        case '*':
            return atom(Token::Kind::Mul);
    }
}

}  // namespace ExpressionParser

}  // namespace P4Tools
