#ifndef IR_SEMANTIC_LESS_H_
#define IR_SEMANTIC_LESS_H_

#include <algorithm>
#include <type_traits>

namespace P4::IR {

/// SFINAE helper to check if given class has a `isSemanticallyLess` method of any form.
template <typename T>
class has_SemanticallyLess {
 private:
    typedef char YesType[1];
    typedef char NoType[2];

    template <typename C>
    static YesType &test(decltype(&C::isSemanticallyLess));
    template <typename C>
    static NoType &test(...);

 public:
    enum { value = sizeof(test<T>(0)) == sizeof(YesType) };
};

template <class T>
inline constexpr bool has_SemanticallyLess_v = has_SemanticallyLess<T>::value;

// Variable template that checks if a type has begin() and end() member functions
template <typename, typename = void>
constexpr bool is_iterable = false;

template <typename T>
constexpr bool is_iterable<
    T, std::void_t<decltype(std::declval<T>().begin()), decltype(std::declval<T>().end())>> = true;

template <class T, typename std::enable_if<std::is_integral<T>::value, T>::type * = nullptr>
bool isSemanticallyLess(const T &a, const T &b) {
    return a < b;
}

template <class T, typename std::enable_if<std::is_enum<T>::value, T>::type * = nullptr>
inline bool isSemanticallyLess(const T &a, const T &b) {
    return a < b;
}

template <class T, typename std::enable_if<std::is_pointer<T>::value, T>::type *>
bool isSemanticallyLess(const T &a, const T &b);

/// TODO: This also includes containers such as safe::vectors.
// These containers may use operator< for comparison.
// Should it be the responsibility of the IR node implementer to ensure they are implementing
// container comparison correctly?
template <class T, typename std::enable_if<std::is_class<T>::value, T>::type * = nullptr>
inline bool isSemanticallyLess(const T &a, const T &b) {
    if constexpr (has_SemanticallyLess_v<T>) {
        return a.isSemanticallyLess(b);
    } else {
        return a < b;
    }
}

template <class T, typename std::enable_if<std::is_pointer<T>::value, T>::type * = nullptr>
inline bool isSemanticallyLess(const T a, const T b) {
    if constexpr (has_SemanticallyLess_v<T>) {
        return a != nullptr ? b != nullptr ? a->isSemanticallyLess(*b) : false : b != nullptr;
    }
    return a != nullptr ? b != nullptr ? IR::isSemanticallyLess(*a, *b) : false : b != nullptr;
}

}  // namespace P4::IR

#endif  // IR_SEMANTIC_LESS_H_
