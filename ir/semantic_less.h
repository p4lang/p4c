#ifndef IR_SEMANTIC_LESS_H_
#define IR_SEMANTIC_LESS_H_

#include <algorithm>
#include <concepts>
#include <functional>
#include <type_traits>

namespace P4::IR {

/// SFINAE helper to check if given class has a `isSemanticallyLess` method of any form.
template <typename T, typename = void>
struct has_SemanticallyLess : std::false_type {};

template <typename T>
struct has_SemanticallyLess<
    T, std::void_t<decltype(std::declval<const T &>().isSemanticallyLess(
           std::declval<const std::remove_cv_t<T> &>()))>> : std::true_type {};

template <class T>
inline constexpr bool has_SemanticallyLess_v = has_SemanticallyLess<std::remove_cv_t<T>>::value;

// Variable template that checks if a type has begin() and end() member functions
template <typename, typename = void>
constexpr bool is_iterable = false;

template <typename T>
constexpr bool is_iterable<
    T, std::void_t<decltype(std::declval<T>().begin()), decltype(std::declval<T>().end())>> = true;

template <typename T>
struct is_pair : std::false_type {};

template <typename T1, typename T2>
struct is_pair<std::pair<T1, T2>> : std::true_type {};

template <class T, typename std::enable_if<std::is_integral<T>::value, T>::type * = nullptr>
bool isSemanticallyLess(const T &a, const T &b) {
    return a < b;
}

template <class T, typename std::enable_if<std::is_enum<T>::value, T>::type * = nullptr>
inline bool isSemanticallyLess(const T &a, const T &b) {
    return a < b;
}

template <class T1, class T2, class U1, class U2>
inline bool isSemanticallyLess(const std::pair<T1, T2> &a, const std::pair<U1, U2> &b);

template <class T,
          typename std::enable_if<std::is_class<T>::value && !is_iterable<T> &&
                                      !is_pair<std::remove_cv_t<T>>::value,
                                  T>::type * = nullptr>
inline bool isSemanticallyLess(const T &a, const T &b);

template <class T,
          typename std::enable_if<is_iterable<T> && !std::is_pointer<T>::value, T>::type * =
              nullptr>
inline bool isSemanticallyLess(const T &a, const T &b);

template <class T, typename std::enable_if<std::is_pointer<T>::value, T>::type * = nullptr>
inline bool isSemanticallyLess(const T a, const T b);

/// TODO: This also includes containers such as safe::vectors.
// These containers may use operator< for comparison.
// Should it be the responsibility of the IR node implementer to ensure they are implementing
// container comparison correctly?
template <class T,
          typename std::enable_if<std::is_class<T>::value && !is_iterable<T> &&
                                      !is_pair<std::remove_cv_t<T>>::value,
                                  T>::type *>
inline bool isSemanticallyLess(const T &a, const T &b) {
    if constexpr (has_SemanticallyLess_v<T>) {
        return a.isSemanticallyLess(b);
    } else if constexpr (requires { { a < b } -> std::convertible_to<bool>; }) {
        return a < b;
    } else {
        return std::less<const void *>{}(static_cast<const void *>(&a),
                                         static_cast<const void *>(&b));
    }
}

template <class T1, class T2, class U1, class U2>
inline bool isSemanticallyLess(const std::pair<T1, T2> &a, const std::pair<U1, U2> &b) {
    return IR::isSemanticallyLess(a.first, b.first) ||
           (!IR::isSemanticallyLess(b.first, a.first) && IR::isSemanticallyLess(a.second, b.second));
}

template <class T,
          typename std::enable_if<is_iterable<T> && !std::is_pointer<T>::value, T>::type *>
inline bool isSemanticallyLess(const T &a, const T &b) {
    if constexpr (has_SemanticallyLess_v<T>) {
        return a.isSemanticallyLess(b);
    }
    auto left = a.begin();
    auto right = b.begin();
    while (left != a.end() && right != b.end()) {
        if (IR::isSemanticallyLess(*left, *right)) return true;
        if (IR::isSemanticallyLess(*right, *left)) return false;
        ++left;
        ++right;
    }
    return left == a.end() && right != b.end();
}

template <class T, typename std::enable_if<std::is_pointer<T>::value, T>::type *>
inline bool isSemanticallyLess(const T a, const T b) {
    if constexpr (has_SemanticallyLess_v<T>) {
        return a != nullptr ? b != nullptr ? a->isSemanticallyLess(*b) : false : b != nullptr;
    }
    return a != nullptr ? b != nullptr ? IR::isSemanticallyLess(*a, *b) : false : b != nullptr;
}

}  // namespace P4::IR

#endif  // IR_SEMANTIC_LESS_H_
