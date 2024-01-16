#ifndef IR_SEMANTIC_LESS_H_
#define IR_SEMANTIC_LESS_H_

#include <type_traits>

namespace IR {

template <class T, typename std::enable_if<std::is_integral<T>::value, T>::type * = nullptr>
bool isSemanticallyLess(const T &a, const T &b) {
    return a < b;
}

template <class T, typename std::enable_if<std::is_enum<T>::value, T>::type * = nullptr>
inline bool isSemanticallyLess(const T &a, const T &b) {
    return a < b;
}

/// TODO: This also includes containers such as safe::vectors.
// These containers may use operator< for comparison.
// Should it be the responsibility of the IR node implementer to ensure they are implementing
// container comparison correctly?
template <class T, typename std::enable_if<std::is_class<T>::value, T>::type * = nullptr>
inline bool isSemanticallyLess(const T &a, const T &b) {
    return a < b;
}

}  // namespace IR

#endif  // IR_SEMANTIC_LESS_H_
