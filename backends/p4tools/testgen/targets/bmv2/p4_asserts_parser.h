#ifndef TESTGEN_TARGETS_BMV2_P4_ASSERTS_PARSER_H_
#define TESTGEN_TARGETS_BMV2_P4_ASSERTS_PARSER_H_

#include <iomanip>
#include <iostream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "ir/ir.h"

namespace P4Tools {
namespace AssertsParser {

class AssertsParser : public Transform {
    std::vector<std::vector<const IR::Expression*>>& restrictionsVec;
    IR::Vector<IR::KeyElement> keyElements;
    cstring tableName;

 public:
    explicit AssertsParser(std::vector<std::vector<const IR::Expression*>>& output);
    std::vector<const IR::Expression*> genIRStructs(cstring str);
    const IR::Node* postorder(IR::P4Table* node) override;
};

class Token {
 public:
    enum class Kind {
        Text,
        True,
        False,
        LineStatementClose,  // \n
        Id,                  // this, this.foo
        Number,              // 1, 2, -1, 5.2, -5.3
        Minus,               // -
        Plus,                // +
        Dot,                 // .
        FieldAcces,          // :: (after key)
        MetadataAccess,      // :: (otherwise)
        LeftParen,           // (
        RightParen,          // )
        Equal,               // ==
        NotEqual,            // !=
        GreaterThan,         // >
        GreaterEqual,        // >=
        LessThan,            // <
        LessEqual,           // <=
        LNot,                // !
        Colon,               // :
        Semicolon,           // ;
        Conjunction,         // &&
        Disjunction,         // ||
        Implication,         // ->
        Slash,               // /
        Percent,             // %
        Shr,                 // >>
        Shl,                 // <<
        Mul,                 // *
        Comment,             // //
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

    Kind kind() const noexcept { return m_kind; }

    void kind(Kind kind) noexcept { m_kind = kind; }

    bool is(Kind kind) const noexcept { return m_kind == kind; }

    bool is_not(Kind kind) const noexcept { return m_kind != kind; }

    bool is_one_of(Kind k1, Kind k2) const noexcept { return is(k1) || is(k2); }

    template <typename... Ts>
    bool is_one_of(Kind k1, Kind k2, Ts... ks) const noexcept {
        return is(k1) || is_one_of(k2, ks...);
    }

    std::string_view lexeme() const noexcept { return m_lexeme; }

    void lexeme(std::string_view lexeme) noexcept { m_lexeme = std::move(lexeme); }
};

class Lexer {
 public:
    explicit Lexer(const char* beg) noexcept : m_beg{beg} {}

    Token next() noexcept;
    const char* m_beg = nullptr;

 private:
    Token atom(Token::Kind) noexcept;

    char peek() const noexcept { return *m_beg; }
    char prev() noexcept { return *m_beg--; }
    char get() noexcept { return *m_beg++; }
};
}  // namespace AssertsParser
}  // namespace P4Tools

#endif /* TESTGEN_TARGETS_BMV2_P4_ASSERTS_PARSER_H_ */
