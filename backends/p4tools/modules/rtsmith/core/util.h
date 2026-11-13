#ifndef BACKENDS_P4TOOLS_MODULES_RTSMITH_CORE_UTIL_H_
#define BACKENDS_P4TOOLS_MODULES_RTSMITH_CORE_UTIL_H_

#include <map>
#include <optional>

namespace P4::P4Tools::RtSmith {

/// If @param inputFunction evaluates to false, returns @param ret.
// NOLINTNEXTLINE
#define RETURN_IF_FALSE(inputFunction, returnValue) \
    if (!(inputFunction)) return returnValue;

/// If @param inputFunction evaluates to false, returns @param ret and also executes @param
/// errorFunction.
// NOLINTNEXTLINE
#define RETURN_IF_FALSE_WITH_MESSAGE(inputFunction, returnValue, errorFunction) \
    if (!(inputFunction)) {                                                     \
        errorFunction;                                                          \
        return returnValue;                                                     \
    }

// Internal helper for concatenating macro values.
#define STATUS_MACROS_CONCAT_NAME_INNER(x, y) x##y
#define STATUS_MACROS_CONCAT_NAME(x, y) STATUS_MACROS_CONCAT_NAME_INNER(x, y)

/// Assigns the value of @param inputFunction to @param targetVariable if the contained value is not
/// false.
// NOLINTNEXTLINE
#define ASSIGN_OR_RETURN_IMPL(temporary, targetVariable, inputFunction, returnValue) \
    auto temporary = inputFunction;                                                  \
    if (!(temporary)) return returnValue;                                            \
    targetVariable = *(temporary);  // NOLINT

#define ASSIGN_OR_RETURN(targetVariable, inputFunction, returnValue)                      \
    ASSIGN_OR_RETURN_IMPL(STATUS_MACROS_CONCAT_NAME(temporary, __LINE__), targetVariable, \
                          inputFunction, returnValue)

#define ASSIGN_OR_RETURN_WITH_MESSAGE_IMPL(temporary, targetVariable, inputFunction, returnValue, \
                                           errorFunction)                                         \
    auto temporary = inputFunction;                                                               \
    if (!(temporary)) {                                                                           \
        errorFunction;                                                                            \
        return returnValue;                                                                       \
    }                                                                                             \
    targetVariable = *(temporary);  // NOLINT

#define ASSIGN_OR_RETURN_WITH_MESSAGE(targetVariable, inputFunction, returnValue, errorFunction) \
    ASSIGN_OR_RETURN_WITH_MESSAGE_IMPL(STATUS_MACROS_CONCAT_NAME(temporary, __LINE__),           \
                                       targetVariable, inputFunction, returnValue, errorFunction)

/// Assigns the value of @param inputFunction to @param targetVariable if the contained value is not
/// false.
/// @param inputFunction is dereferenced, which will work on std::nullopt and pointers, but not
/// bools. Prints an error provided with @param errorFunction if the inputFunction evaluates to
/// false.
// NOLINTNEXTLINE

/// Tries to look up a key in a map, returns std::nullopt if the key does not exist.
template <class K, class T, class V, class Comp, class Alloc>
inline std::optional<V> safeAt(const std::map<K, V, Comp, Alloc> &m, T key) {
    auto it = m.find(key);
    if (it != m.end()) {
        return it->second;
    }
    return std::nullopt;
}

}  // namespace P4::P4Tools::RtSmith

#endif /* BACKENDS_P4TOOLS_MODULES_RTSMITH_CORE_UTIL_H_ */
