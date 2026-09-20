/*
 * SPDX-FileCopyrightText: 2026 The P4 Language Consortium
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef IR_STRUCTURAL_COMPARE_H_
#define IR_STRUCTURAL_COMPARE_H_

#include <compare>
#include <concepts>
#include <iterator>
#include <optional>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>

namespace P4::IR {

namespace detail {
template <typename T>
inline constexpr bool is_variant = false;
template <typename... Ts>
inline constexpr bool is_variant<std::variant<Ts...>> = true;
template <typename T>
inline constexpr bool is_optional = false;
template <typename T>
inline constexpr bool is_optional<std::optional<T>> = true;
}  // namespace detail

/// Compare values recursively, dereferencing pointers (null sorts first). Containers are
/// compared in iteration order, and variants by alternative index followed by their value.
/// A custom member takes precedence over container traversal and scalar comparison.
/// Unsupported field types must provide a value comparison; never fall back to addresses.
/// See docs/IR.md for the contract and metadata excluded by particular IR nodes.
template <typename T>
std::weak_ordering structuralCompare(const T &a, const T &b) {
    if constexpr (std::is_pointer_v<T>) {
        if (a == b) return std::weak_ordering::equivalent;
        if (!a) return std::weak_ordering::less;
        if (!b) return std::weak_ordering::greater;
        return IR::structuralCompare(*a, *b);
    } else if constexpr (requires {
                             { a.structuralCompare(b) } -> std::convertible_to<std::weak_ordering>;
                         }) {
        return a.structuralCompare(b);
    } else if constexpr (detail::is_optional<T>) {
        if (a.has_value() != b.has_value()) return a.has_value() <=> b.has_value();
        return a ? IR::structuralCompare(*a, *b) : std::weak_ordering::equivalent;
    } else if constexpr (detail::is_variant<T>) {
        if (a.index() != b.index()) return a.index() <=> b.index();
        auto result = std::weak_ordering::equivalent;
        if (!a.valueless_by_exception()) {
            [&]<std::size_t... I>(std::index_sequence<I...>) {
                (void)((a.index() == I &&
                        (result = IR::structuralCompare(std::get<I>(a), std::get<I>(b)), true)) ||
                       ...);
            }(std::make_index_sequence<std::variant_size_v<T>>{});
        }
        return result;
    } else if constexpr (requires { std::tuple_size<T>::value; }) {
        return [&]<std::size_t... I>(std::index_sequence<I...>) {
            auto result = std::weak_ordering::equivalent;
            // Compare each element once and stop at the first difference.
            (void)(((result = IR::structuralCompare(std::get<I>(a), std::get<I>(b))) != 0) || ...);
            return result;
        }(std::make_index_sequence<std::tuple_size_v<T>>{});
    } else if constexpr (!std::is_array_v<T> &&
                         std::is_convertible_v<const T &, std::string_view>) {
        if constexpr (requires { a <=> b; }) {
            return a <=> b;
        } else {
            // cstring equality is constant-time; preserve its null/empty distinction.
            if (a == b) return std::weak_ordering::equivalent;
            return a < b ? std::weak_ordering::less : std::weak_ordering::greater;
        }
    } else if constexpr (requires {
                             std::begin(a);
                             std::end(a);
                         }) {
        // Some IR support types have nonassignable iterators.
        auto left = std::begin(a);
        auto right = std::begin(b);
        const auto leftEnd = std::end(a);
        const auto rightEnd = std::end(b);
        while (left != leftEnd && right != rightEnd) {
            if (auto result = IR::structuralCompare(*left, *right); result != 0) return result;
            ++left;
            ++right;
        }
        return (right == rightEnd) <=> (left == leftEnd);
    } else if constexpr (requires {
                             { a <=> b } -> std::convertible_to<std::weak_ordering>;
                         }) {
        return a <=> b;
    } else if constexpr (requires {
                             { a.compare(b) } -> std::convertible_to<int>;
                         }) {
        // Multiprecision integers can determine all three outcomes in one pass.
        return a.compare(b) <=> 0;
    } else if constexpr (requires {
                             { a < b } -> std::convertible_to<bool>;
                         }) {
        if (a < b) return std::weak_ordering::less;
        if (b < a) return std::weak_ordering::greater;
        return std::weak_ordering::equivalent;
    } else {
        static_assert(!std::is_same_v<T, T>, "IR field needs a structural value comparison");
    }
}

}  // namespace P4::IR

#endif  // IR_STRUCTURAL_COMPARE_H_
