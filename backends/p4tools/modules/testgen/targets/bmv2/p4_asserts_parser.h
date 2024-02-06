#ifndef BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_BMV2_P4_ASSERTS_PARSER_H_
#define BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_BMV2_P4_ASSERTS_PARSER_H_

#include <cstddef>
#include <iterator>
#include <string_view>
#include <vector>

#include "backends/p4tools/common/lib/variables.h"
#include "ir/ir.h"
#include "ir/node.h"
#include "ir/vector.h"
#include "ir/visitor.h"
#include "lib/cstring.h"

namespace P4Tools::P4Testgen::Bmv2 {

/// Maps control plane identifiers to their corresponding IR type.
/// Once we have resolved the tokens in the annotation, we can then lookup the corresponding type.
using IdenitifierTypeMap = std::map<cstring, const IR::Type *>;

class AssertsParser : public Transform {
    /// A vector of restrictions imposed on the control-plane.
    ConstraintsVector &restrictionsVec;

 public:
    explicit AssertsParser(ConstraintsVector &output);
    /// A function that calls the beginning of the transformation of restrictions from a string into
    /// an IR::Expression. Internally calls all other necessary functions, for example
    /// combineTokensToNames and the like, to eventually get an IR expression that meets the string
    /// constraint
    static std::vector<const IR::Expression *> genIRStructs(cstring tableName,
                                                            cstring restrictionString,
                                                            const IdenitifierTypeMap &typeMap);
    const IR::Node *postorder(IR::P4Action *actionContext) override;
    const IR::Node *postorder(IR::P4Table *tableContext) override;
};

class Token {
 public:
    enum class Kind {
        Priority,
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

    Token(Kind kind, const char *beg, std::size_t len) noexcept
        : m_kind{kind}, m_lexeme(beg, len) {}

    Token(Kind kind, const char *beg, const char *end) noexcept
        : m_kind{kind}, m_lexeme(beg, std::distance(beg, end)) {}

    [[nodiscard]] Kind kind() const noexcept;

    void kind(Kind kind) noexcept;

    [[nodiscard]] bool is(Kind kind) const noexcept;

    [[nodiscard]] bool isNot(Kind kind) const noexcept;

    [[nodiscard]] bool isOneOf(Kind k1, Kind k2) const noexcept;

    template <typename... Ts>
    bool isOneOf(Kind k1, Kind k2, Ts... ks) const noexcept;

    [[nodiscard]] std::string_view lexeme() const noexcept;

    void lexeme(std::string_view lexeme) noexcept;
};

class Lexer {
 public:
    explicit Lexer(const char *beg) noexcept : mBeg{beg} {}

    Token next() noexcept;
    const char *mBeg = nullptr;

 private:
    Token atom(Token::Kind) noexcept;

    [[nodiscard]] char peek() const noexcept;
    char prev() noexcept;
    char get() noexcept;
};
}  // namespace P4Tools::P4Testgen::Bmv2

#endif /* BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_BMV2_P4_ASSERTS_PARSER_H_ */
