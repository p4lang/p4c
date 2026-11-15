// SPDX-FileCopyrightText: 2026 The P4 Language Consortium & Devansh Singh
// SPDX-License-Identifier: Apache-2.0

#include <gtest/gtest.h>

#include <cctype>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <random>
#include <string>
#include <utility>
#include <vector>

#include "frontends/p4/frontend.h"
#include "frontends/p4/unusedDeclarations.h"
#include "helpers.h"
#include "ir/ir.h"
#include "lib/big_int_util.h"

namespace P4::Test {

namespace {

enum class Kind { Bit, Int, Bool };

/// A generated expression: its P4 source and the value it must evaluate to.
struct Expr {
    Kind kind = Kind::Bool;
    unsigned width = 0;  // 0 for Bool.
    big_int value = 0;   // 0 or 1 for Bool.
    std::string text;
    /// For a plain non-negative literal: the same literal without its type, e.g. "5" for "8w5".
    std::string untyped;

    std::string typeName() const {
        switch (kind) {
            case Kind::Bit:
                return "bit<" + std::to_string(width) + ">";
            case Kind::Int:
                return "int<" + std::to_string(width) + ">";
            case Kind::Bool:
                break;
        }
        return "bool";
    }
};

big_int pow2(unsigned n) { return big_int(1) << n; }

big_int wrapUnsigned(const big_int &v, unsigned width) {
    const big_int modulus = pow2(width);
    big_int r = v % modulus;
    if (r < 0) r += modulus;
    return r;
}

big_int wrapSigned(const big_int &v, unsigned width) {
    const big_int r = wrapUnsigned(v, width);
    return r >= pow2(width - 1) ? r - pow2(width) : r;
}

big_int wrap(Kind kind, const big_int &v, unsigned width) {
    return kind == Kind::Int ? wrapSigned(v, width) : wrapUnsigned(v, width);
}

big_int minValue(Kind kind, unsigned width) { return kind == Kind::Int ? -pow2(width - 1) : 0; }

big_int maxValue(Kind kind, unsigned width) {
    return kind == Kind::Int ? pow2(width - 1) - 1 : pow2(width) - 1;
}

/// The two's complement bit pattern of the value, as a non-negative number.
big_int bitsOf(const Expr &e) { return wrapUnsigned(e.value, e.width); }

/// Right shift that rounds towards negative infinity, like a two's complement shift.
big_int arithmeticShiftRight(const big_int &v, unsigned amount) {
    if (v >= 0) return v >> amount;
    return -(((-v) - 1) >> amount) - 1;
}

/// A top-level declaration the generated expressions refer to.
struct Declaration {
    std::string name;
    std::string text;
};

bool mentions(const std::string &text, const std::string &name) {
    const auto isIdentifier = [](char c) {
        return std::isalnum(static_cast<unsigned char>(c)) || c == '_';
    };
    for (auto pos = text.find(name); pos != std::string::npos; pos = text.find(name, pos + 1)) {
        const bool startsWord = pos == 0 || !isIdentifier(text[pos - 1]);
        const bool endsWord =
            pos + name.size() == text.size() || !isIdentifier(text[pos + name.size()]);
        if (startsWord && endsWord) return true;
    }
    return false;
}

/// The declarations that @p text needs, directly or through other declarations, in order.
std::vector<Declaration> relevantDeclarations(const std::vector<Declaration> &decls,
                                              const std::string &text) {
    std::vector<bool> needed(decls.size(), false);
    std::vector<std::string> pending = {text};
    while (!pending.empty()) {
        const std::string current = pending.back();
        pending.pop_back();
        for (size_t i = 0; i < decls.size(); i++) {
            if (needed[i] || !mentions(current, decls[i].name)) continue;
            needed[i] = true;
            pending.push_back(decls[i].text);
        }
    }
    std::vector<Declaration> result;
    for (size_t i = 0; i < decls.size(); i++)
        if (needed[i]) result.push_back(decls[i]);
    return result;
}

const std::vector<unsigned> widths = {1,  2,  3,  4,  7,  8,  9,   15,  16,
                                      31, 32, 33, 63, 64, 65, 100, 128, 200};

class Generator {
 public:
    struct Options {
        /// Route some literals through (possibly nested) fields of constant structs.
        bool structMembers = false;
        /// Use untyped literals as operands, which the type checker has to cast implicitly.
        bool untypedLiterals = false;
    };

    Generator(uint64_t seed, Options options) : rng(seed), options(options) {}

    /// Declarations needed by the expressions generated since the last reset().
    const std::vector<Declaration> &declarations() const { return decls; }

    void reset() {
        decls.clear();
        memberCount = 0;
    }

    /// An expression of random kind and width.
    Expr any(unsigned depth) {
        static const Kind kinds[] = {Kind::Bit, Kind::Bit, Kind::Int, Kind::Bool};
        const Kind kind = kinds[below(4)];
        return generate(kind, kind == Kind::Bool ? 0 : pick(widths), depth);
    }

 private:
    std::mt19937_64 rng;
    Options options;
    std::vector<Declaration> decls;
    unsigned memberCount = 0;

    unsigned below(unsigned n) { return static_cast<unsigned>(rng() % n); }
    bool chance(unsigned percent) { return below(100) < percent; }
    template <typename T>
    const T &pick(const std::vector<T> &v) {
        return v[below(static_cast<unsigned>(v.size()))];
    }

    big_int randomBits(unsigned width) {
        big_int r = 0;
        for (unsigned i = 0; i < width; i += 32) r = (r << 32) | big_int(rng() & 0xffffffffU);
        return wrapUnsigned(r, width);
    }

    void declare(const std::string &name, const std::string &text) {
        for (const auto &d : decls)
            if (d.name == name) return;
        decls.push_back({name, text});
    }

    Expr literal(Kind kind, unsigned width, const big_int &v) {
        Expr e;
        e.kind = kind;
        e.width = width;
        e.value = v;
        const std::string w = std::to_string(width);
        if (kind == Kind::Bit) {
            e.text = w + "w" + v.str();
        } else if (v >= 0) {
            e.text = w + "s" + v.str();
        } else if (-v <= maxValue(Kind::Int, width)) {
            e.text = "(-" + w + "s" + big_int(-v).str() + ")";
        } else {
            // The most negative value has no literal of its own: reinterpret its bit pattern.
            e.text = "((int<" + w + ">)" + w + "w" + pow2(width - 1).str() + ")";
        }
        if (v >= 0) e.untyped = v.str();
        return e;
    }

    /// Edge cases of the type, plus a random value.
    big_int edgeValue(Kind kind, unsigned width) {
        std::vector<big_int> pool;
        const big_int lo = minValue(kind, width), hi = maxValue(kind, width);
        pool = {0, 1, hi, hi - 1, lo, lo + 1, kind == Kind::Int ? big_int(-1) : pow2(width - 1)};
        pool.push_back(wrap(kind, randomBits(width), width));
        std::vector<big_int> valid;
        for (const auto &v : pool)
            if (v >= lo && v <= hi) valid.push_back(v);
        return pick(valid);
    }

    Expr leaf(Kind kind, unsigned width) {
        if (kind == Kind::Bool) {
            Expr e;
            e.kind = Kind::Bool;
            e.value = below(2);
            e.text = e.value != 0 ? "true" : "false";
            return e;
        }
        Expr e = literal(kind, width, edgeValue(kind, width));
        if (options.structMembers && chance(35)) return viaStructMember(e);
        return e;
    }

    /// Replaces a literal by a field of a constant struct initialized with it.
    Expr viaStructMember(const Expr &base) {
        const std::string tag = (base.kind == Kind::Bit ? "b" : "i") + std::to_string(base.width);
        const std::string inner = "S" + tag;
        declare(inner, "struct " + inner + " { " + base.typeName() + " f; }");
        const std::string name = "m" + std::to_string(++memberCount);
        Expr e = base;
        e.untyped.clear();
        if (chance(30)) {
            const std::string outer = "O" + tag;
            declare(outer, "struct " + outer + " { " + inner + " i; }");
            declare(name, "const " + outer + " " + name + " = { i = { f = " + base.text + " } };");
            e.text = name + ".i.f";
        } else {
            declare(name, "const " + inner + " " + name + " = { f = " + base.text + " };");
            e.text = name + ".f";
        }
        return e;
    }

    /// Sometimes writes a non-negative literal without its type.
    Expr maybeUntyped(Expr e) {
        if (options.untypedLiterals && !e.untyped.empty() && chance(50)) e.text = e.untyped;
        return e;
    }

    Expr generate(Kind kind, unsigned width, unsigned depth) {
        if (depth == 0 || chance(20)) return leaf(kind, width);
        if (kind == Kind::Bool) return boolean(depth);
        switch (below(11)) {
            case 0:
            case 1:
                return binary(kind, width, depth);
            case 2:
                return unary(kind, width, depth);
            case 3:
                return shift(kind, width, depth);
            case 4:
                return saturating(kind, width, depth);
            case 5:
                return cast(kind, width, depth);
            case 6:
                return ternary(kind, width, depth);
            case 7:
                return slice(kind, width, depth);
            case 8:
                return concat(kind, width, depth);
            case 9:
                return divMod(kind, width, depth);
            default:
                return leaf(kind, width);
        }
    }

    static Expr make(Kind kind, unsigned width, const big_int &value, std::string text) {
        Expr e;
        e.kind = kind;
        e.width = width;
        e.value = value;
        e.text = std::move(text);
        return e;
    }

    Expr binary(Kind kind, unsigned width, unsigned depth) {
        const Expr a = generate(kind, width, depth - 1);
        const Expr b = maybeUntyped(generate(kind, width, depth - 1));
        static const char *const ops[] = {"+", "-", "*", "&", "|", "^"};
        const unsigned op = below(6);
        big_int v;
        switch (op) {
            case 0:
                v = a.value + b.value;
                break;
            case 1:
                v = a.value - b.value;
                break;
            case 2:
                v = a.value * b.value;
                break;
            case 3:
                v = bitsOf(a) & bitsOf(b);
                break;
            case 4:
                v = bitsOf(a) | bitsOf(b);
                break;
            default:
                v = bitsOf(a) ^ bitsOf(b);
                break;
        }
        return make(kind, width, wrap(kind, v, width),
                    "(" + a.text + " " + ops[op] + " " + b.text + ")");
    }

    Expr unary(Kind kind, unsigned width, unsigned depth) {
        const Expr a = generate(kind, width, depth - 1);
        if (chance(50))
            return make(kind, width, wrap(kind, (pow2(width) - 1) ^ bitsOf(a), width),
                        "(~" + a.text + ")");
        return make(kind, width, wrap(kind, -a.value, width), "(-" + a.text + ")");
    }

    Expr shift(Kind kind, unsigned width, unsigned depth) {
        const Expr a = generate(kind, width, depth - 1);
        const unsigned amounts[] = {0, 1, width - 1, width, width + 1, below(width + 3)};
        unsigned amount = amounts[below(6)];
        std::string amountText = std::to_string(amount);
        if (chance(50)) {
            if (amount > 255) amount = 255;
            amountText = "8w" + std::to_string(amount);
        }
        if (chance(50)) {
            return make(kind, width, wrap(kind, a.value * pow2(amount), width),
                        "(" + a.text + " << " + amountText + ")");
        }
        const big_int v =
            kind == Kind::Int ? arithmeticShiftRight(a.value, amount) : big_int(a.value >> amount);
        return make(kind, width, wrap(kind, v, width), "(" + a.text + " >> " + amountText + ")");
    }

    Expr saturating(Kind kind, unsigned width, unsigned depth) {
        const Expr a = generate(kind, width, depth - 1);
        const Expr b = maybeUntyped(generate(kind, width, depth - 1));
        const bool add = chance(50);
        big_int v = add ? big_int(a.value + b.value) : big_int(a.value - b.value);
        if (v < minValue(kind, width)) v = minValue(kind, width);
        if (v > maxValue(kind, width)) v = maxValue(kind, width);
        return make(kind, width, v, "(" + a.text + (add ? " |+| " : " |-| ") + b.text + ")");
    }

    Expr ternary(Kind kind, unsigned width, unsigned depth) {
        const Expr c = generate(Kind::Bool, 0, depth - 1);
        const Expr a = generate(kind, width, depth - 1);
        const Expr b = generate(kind, width, depth - 1);
        return make(kind, width, c.value != 0 ? a.value : b.value,
                    "(" + c.text + " ? " + a.text + " : " + b.text + ")");
    }

    Expr cast(Kind kind, unsigned width, unsigned depth) {
        const Kind sourceKind = chance(50) ? Kind::Bit : Kind::Int;
        // Casts between different signedness keep the width.
        const unsigned sourceWidth = sourceKind == kind ? pick(widths) : width;
        const Expr a = generate(sourceKind, sourceWidth, depth - 1);
        return make(kind, width, wrap(kind, a.value, width),
                    "((" + Expr{kind, width, 0, "", ""}.typeName() + ")" + a.text + ")");
    }

    Expr slice(Kind kind, unsigned width, unsigned depth) {
        const Kind sourceKind = chance(50) ? Kind::Bit : Kind::Int;
        std::vector<unsigned> wider;
        for (unsigned w : widths)
            if (w >= width) wider.push_back(w);
        const unsigned sourceWidth = pick(wider);
        const Expr a = generate(sourceKind, sourceWidth, depth - 1);
        const unsigned lo = below(sourceWidth - width + 1);
        const std::string range =
            "[" + std::to_string(lo + width - 1) + ":" + std::to_string(lo) + "]";
        const big_int bits = (bitsOf(a) >> lo) & (pow2(width) - 1);
        if (kind == Kind::Int)
            return make(kind, width, wrapSigned(bits, width),
                        "((int<" + std::to_string(width) + ">)(" + a.text + range + "))");
        return make(kind, width, bits, "(" + a.text + range + ")");
    }

    Expr concat(Kind kind, unsigned width, unsigned depth) {
        if (kind != Kind::Bit || width < 2) return leaf(kind, width);
        const unsigned leftWidth = 1 + below(width - 1);
        const Expr a = generate(Kind::Bit, leftWidth, depth - 1);
        const Expr b = generate(Kind::Bit, width - leftWidth, depth - 1);
        return make(kind, width, (a.value << (width - leftWidth)) + b.value,
                    "(" + a.text + " ++ " + b.text + ")");
    }

    Expr divMod(Kind kind, unsigned width, unsigned depth) {
        if (kind != Kind::Bit) return leaf(kind, width);
        const Expr a = generate(kind, width, depth - 1);
        const Expr b = generate(kind, width, depth - 1);
        if (b.value == 0) return leaf(kind, width);
        const bool div = chance(50);
        return make(kind, width, div ? big_int(a.value / b.value) : big_int(a.value % b.value),
                    "(" + a.text + (div ? " / " : " % ") + b.text + ")");
    }

    Expr boolean(unsigned depth) {
        const unsigned choice = below(100);
        if (choice < 50) {
            const Kind kind = chance(50) ? Kind::Bit : Kind::Int;
            const unsigned width = pick(widths);
            const Expr a = generate(kind, width, depth - 1);
            const Expr b = maybeUntyped(generate(kind, width, depth - 1));
            static const char *const ops[] = {"<", "<=", ">", ">=", "==", "!="};
            const unsigned op = below(6);
            const bool results[] = {a.value<b.value, a.value <= b.value, a.value> b.value,
                                    a.value >= b.value, a.value == b.value, a.value != b.value};
            return make(Kind::Bool, 0, results[op] ? 1 : 0,
                        "(" + a.text + " " + ops[op] + " " + b.text + ")");
        }
        if (choice < 80) {
            const Expr a = generate(Kind::Bool, 0, depth - 1);
            const Expr b = generate(Kind::Bool, 0, depth - 1);
            const bool both = chance(50);
            const bool r = both ? (a.value != 0 && b.value != 0) : (a.value != 0 || b.value != 0);
            return make(Kind::Bool, 0, r ? 1 : 0,
                        "(" + a.text + (both ? " && " : " || ") + b.text + ")");
        }
        const Expr a = generate(Kind::Bool, 0, depth - 1);
        return make(Kind::Bool, 0, a.value != 0 ? 0 : 1, "(!" + a.text + ")");
    }
};

/// What the frontend folded a constant to.
struct Folded {
    Kind kind;
    unsigned width;
    big_int value;
};

/// Keeps the constants the test inspects, which would otherwise be removed as unused.
class KeepUnusedPolicy : public P4::FrontEndPolicy {
 public:
    P4::UnusedDeclarations *getUnusedDeclarationsPass(const P4::UsedDeclSet &used,
                                                      bool /* removeUnused */,
                                                      bool /* warnUnused */,
                                                      bool /* infoRemoved */) const override {
        return RemoveUnusedPolicy::getUnusedDeclarationsPass(used, false, false, false);
    }
};

std::string render(const std::vector<Declaration> &decls, const std::vector<Expr> &exprs) {
    std::string source;
    for (const auto &d : decls) source += d.text + "\n";
    for (size_t i = 0; i < exprs.size(); i++)
        source += "const " + exprs[i].typeName() + " c" + std::to_string(i) + " = " +
                  exprs[i].text + ";\n";
    return source;
}

/// Runs the frontend on @p source. Returns std::nullopt if it rejects the program, otherwise for
/// each constant c0, c1, ... what it was folded to (std::nullopt if it is not a constant).
std::optional<std::vector<std::optional<Folded>>> evaluate(const std::string &source,
                                                           size_t count) {
    // Diagnostics accumulate in the context, so every program gets a fresh one.
    AutoCompileContext autoContext(std::make_unique<GTestContext>());
    auto test = FrontendTestCase::create(
        P4_SOURCE(source.c_str()), CompilerOptions::FrontendVersion::P4_16, new KeepUnusedPolicy());
    if (!test) return std::nullopt;
    std::vector<std::optional<Folded>> result(count);
    for (const auto *object : test->program->objects) {
        const auto *decl = object->to<IR::Declaration_Constant>();
        if (decl == nullptr || decl->name.name.isNullOrEmpty() || *decl->name.name.c_str() != 'c')
            continue;
        const size_t index = std::stoul(decl->name.name.c_str() + 1);
        if (index >= count) continue;
        if (const auto *c = decl->initializer->to<IR::Constant>()) {
            const auto *type = c->type->to<IR::Type_Bits>();
            if (type == nullptr) continue;
            result[index] = Folded{type->isSigned ? Kind::Int : Kind::Bit,
                                   static_cast<unsigned>(type->size), c->value};
        } else if (const auto *b = decl->initializer->to<IR::BoolLiteral>()) {
            result[index] = Folded{Kind::Bool, 0, b->value ? 1 : 0};
        }
    }
    return result;
}

std::string describe(const Expr &e) { return e.typeName() + " " + e.value.str(); }

std::string describe(const std::optional<Folded> &f) {
    if (!f) return "<not folded to a constant>";
    return describe(Expr{f->kind, f->width, f->value, "", ""});
}

/// Generates @p batches programs with @p perBatch constants each and checks them all.
void checkEvaluation(uint64_t seed, Generator::Options options, unsigned batches, unsigned perBatch,
                     unsigned depth) {
    constexpr unsigned maxReports = 3;
    unsigned reports = 0;
    Generator generator(seed, options);
    for (unsigned batch = 0; batch < batches && reports < maxReports; batch++) {
        generator.reset();
        std::vector<Expr> exprs;
        for (unsigned i = 0; i < perBatch; i++) exprs.push_back(generator.any(depth));

        auto folded = evaluate(render(generator.declarations(), exprs), exprs.size());
        if (!folded) {
            // Find the expressions that make the frontend reject the program.
            for (const auto &e : exprs) {
                const std::vector<Expr> single = {e};
                const std::string source =
                    render(relevantDeclarations(generator.declarations(), e.text), single);
                if (evaluate(source, 1) || reports >= maxReports) continue;
                reports++;
                ADD_FAILURE() << "The frontend rejects a valid program (seed " << seed << ", batch "
                              << batch << "):\n"
                              << source;
            }
            continue;
        }
        for (size_t i = 0; i < exprs.size() && reports < maxReports; i++) {
            const auto &actual = (*folded)[i];
            if (actual && actual->kind == exprs[i].kind && actual->width == exprs[i].width &&
                actual->value == exprs[i].value)
                continue;
            reports++;
            ADD_FAILURE() << "Wrong value (seed " << seed << ", batch " << batch << ")\n  const "
                          << exprs[i].typeName() << " c = " << exprs[i].text << ";\n  expected "
                          << describe(exprs[i]) << "\n  got      " << describe(actual);
        }
    }
}

}  // namespace

TEST(ConstantEvaluation, TypedLiterals) { checkEvaluation(1, {false, false}, 15, 25, 3); }

TEST(ConstantEvaluation, UntypedLiterals) { checkEvaluation(2, {false, true}, 15, 25, 3); }

TEST(ConstantEvaluation, StructMembers) { checkEvaluation(3, {true, false}, 6, 25, 3); }

TEST(ConstantEvaluation, StructMembersAndUntypedLiterals) {
    checkEvaluation(4, {true, true}, 12, 25, 3);
}

TEST(ConstantEvaluation, DeepExpressions) { checkEvaluation(5, {true, true}, 5, 20, 5); }

}  // namespace P4::Test
