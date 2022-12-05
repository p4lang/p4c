#ifndef TESTGEN_TARGETS_BMV2_P4_ASSERTS_PARSER_H_
#define TESTGEN_TARGETS_BMV2_P4_ASSERTS_PARSER_H_

#include <cstddef>
#include <iterator>
#include <set>
#include <string_view>
#include <vector>

#include "ir/ir.h"
#include "ir/node.h"
#include "ir/vector.h"
#include "ir/visitor.h"
#include "lib/cstring.h"

namespace P4Tools {
namespace AssertsParser {

class AssertsParser : public Transform {
    std::vector<std::vector<const IR::Expression*>>& restrictionsVec;

 public:
    explicit AssertsParser(std::vector<std::vector<const IR::Expression*>>& output);
    /// A function that calls the beginning of the transformation of restrictions from a string into
    /// an IR::Expression. Internally calls all other necessary functions, for example
    /// combineTokensToNames and the like, to eventually get an IR expression that meets the string
    /// constraint
    static std::vector<const IR::Expression*> genIRStructs(
        cstring tableName, cstring restrictionString,
        const IR::Vector<IR::KeyElement>& keyElements);
    const IR::Node* postorder(IR::P4Table* node) override;
};

class Token {
 public:
    enum class Kind {
        Priority,
        Text,
        LineStatementClose,  // \n
        Number,              // 1, 2, -1, 5.2, -5.3
        Comment,             // //String
        StringLiteral,       // "String"
        LeftParen,           // (
        RightParen,          // )
        LeftSParent,         // [
        RightSParent,        // ]
        Dot,                 // .
        FieldAccess,         // ::
        LNot,                // !
        Complement,          // ~
        Mul,                 // *
        Percent,             // %
        Slash,               // /
        Minus,               // -
        SaturationSub,       // |-|
        Plus,                // +
        SaturationAdd,       // |+|
        LessEqual,           // <=
        Shl,                 // <<
        LessThan,            // <
        GreaterEqual,        // >=
        Shr,                 // >>
        GreaterThan,         // >
        NotEqual,            // !=
        Equal,               // ==
        BAnd,                // &
        Xor,                 // ^
        BOr,                 // |
        Conjunction,         // &&
        Disjunction,         // ||
        Implication,         // ->
        Colon,               // :
        Question,            // ?
        Semicolon,           // ;
        Comma,               // ,
        Unknown,
        EndString,
        End,
    };

    Kind m_kind{};
    std::string_view m_lexeme{};
    explicit Token(Kind kind) noexcept : m_kind{kind} {}

    Token(Kind kind, const char* beg, std::size_t len) noexcept
        : m_kind{kind}, m_lexeme(beg, len) {}

    Token(Kind kind, const char* beg, const char* end) noexcept
        : m_kind{kind}, m_lexeme(beg, std::distance(beg, end)) {}

    Kind kind() const noexcept;

    void kind(Kind kind) noexcept;

    bool is(Kind kind) const noexcept;

    bool is_not(Kind kind) const noexcept;

    bool is_one_of(Kind k1, Kind k2) const noexcept;

    template <typename... Ts>
    bool is_one_of(Kind k1, Kind k2, Ts... ks) const noexcept;

    std::string_view lexeme() const noexcept;

    void lexeme(std::string_view lexeme) noexcept;
};

class Lexer {
 public:
    explicit Lexer(const char* beg) noexcept : m_beg{beg} {}

    Token next() noexcept;
    const char* m_beg = nullptr;

 private:
    Token atom(Token::Kind) noexcept;

    char peek() const noexcept;
    char prev() noexcept;
    char get() noexcept;
};

using TokensSet = std::set<Token::Kind>;

class Parser {
 private:
    const IR::P4Program* program;
    const std::vector<Token> tokens;
    size_t index;

 public:
    Parser(const IR::P4Program* program, std::vector<Token> &tokens);
    static const IR::Expression* getIR(const char* str, const IR::P4Program* program,
                                       TokensSet skippedTokens = {Token::Kind::Comment});
    char prev() noexcept;
    
 protected:
    const IR::Expression* getIR();
    const IR::Expression* createPunctuationMarks();
    const IR::Expression* createListExpressions(const IR::Expression*, const char*, Token::Kind);
    const IR::Expression* createLogicalOp();
    const IR::Expression* createIR(Token::Kind, const IR::Expression*, const IR::Expression*);
    const IR::Expression* createBinaryOp();
    const IR::Expression* createEqCompareAndShiftOp();
    const IR::Expression* createArithmeticOp();
    const IR::Expression* createFunctionCallOrConstantOp();
    const IR::Expression* createConstantOp();
    const IR::Expression* createSliceOrArrayOp(const IR::Expression*);
    const IR::Type* getDefinedType(cstring txt, const IR::Type* prevType);
};

}  // namespace AssertsParser
}  // namespace P4Tools

#endif /* TESTGEN_TARGETS_BMV2_P4_ASSERTS_PARSER_H_ */
