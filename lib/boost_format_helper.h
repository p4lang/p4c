#ifndef LIB_BOOST_FORMAT_HELPER_H_
#define LIB_BOOST_FORMAT_HELPER_H_

#include <absl/strings/string_view.h>

#include <charconv>
#include <cstring>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <tuple>
#include <type_traits>
#include <utility>

#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"
#include "lib/cstring.h"
#include "lib/source_file.h"
#include "lib/stringify.h"

namespace P4 {

namespace detail {

template <typename T>
constexpr bool is_char_pointer_v = std::is_same_v<T, char *> || std::is_same_v<T, const char *>;

template <typename T>
constexpr bool is_void_pointer_v = std::is_same_v<T, void *> || std::is_same_v<T, const void *>;
}  // namespace detail

// Concept to check if a type T has a toString() member function
// returning something convertible to absl::string_view
template <typename T>
concept HasToStringMethod = requires(const T &t) {
    // Check if t.toString() is a valid expression
    // and its result is convertible to absl::string_view
    { t.toString() } -> std::convertible_to<absl::string_view>;
};

// Concept to check if a type T can be streamed using operator<<
template <typename T>
concept HasStreamOperator = requires(const T &t, std::ostringstream &os) {
    // Check if os << t is a valid expression
    os << t;
};

// Define the generic AbslStringify template.
// It's constrained to only instantiate for types T that satisfy
// EITHER HasToStringMethod OR HasStreamOperator,
// but NOT types that Abseil already handles intrinsically (like string, int etc)
// or types that might have their *own* specific AbslStringify overload.
template <typename Sink, typename T>
// Add constraint: requires the type has one of the methods we want to handle
    requires(HasToStringMethod<T> || HasStreamOperator<T>)
// Optional: Add constraint to avoid conflicts with fundamental types if needed,
// though overload resolution usually handles this.
// && (!std::is_fundamental_v<T> && !std::is_same_v<T, std::string> ...)
void AbslStringify(Sink &sink, const T &value) {
    // Use 'if constexpr' to choose the implementation at compile time
    if constexpr (HasToStringMethod<T>) {
        // Priority 1: Use toString() if available
        sink.Append(value.toString());
    } else if constexpr (HasStreamOperator<T>) {
        // Priority 2: Use operator<< if toString() is not available
        std::ostringstream oss;
        oss << value;
        // Note: This involves creating a temporary string.
        // If performance is critical for streamable types,
        // providing a dedicated AbslStringify might be better.
        sink.Append(oss.str());
    }
    // Due to the 'requires' clause, one of the above branches MUST be taken.
    // A static_assert(false, ...) here would be technically correct but guarded
    // by unreachable conditions.
}

// // This is the Abseil extension point. It MUST have this signature.
// // It will be found via ADL if the type T is in namespace P4, or if it's
// // called for a type in another namespace that includes this header.
// // If types needing custom formatting are outside P4, you might need to define
// // AbslStringify in *their* namespace(s) as well, forwarding to detail::streamArgument.
// template <typename Sink, typename T>
//     requires(!detail::is_char_pointer_v<T>)
// void AbslStringify(Sink &sink, const T &value) {
//     // Use decay_t for consistent type checking
//     using ArgType = std::decay_t<T>;
//     throw std::runtime_error(absl::StrCat(
//         "Internal error: Invalid argument type in streamArgument: ", typeid(T).name()));
//     printf("AbslStringify: %s\n", typeid(T).name());
//     // 1. Handle SourceInfo (Filter)
//     if constexpr (std::is_same_v<ArgType, P4::Util::SourceInfo>) {
//         // Append nothing for SourceInfo
//         return;
//     }
//     // 2. Handle Pointers
//     if constexpr (std::is_pointer_v<ArgType>) {
//         if (value == nullptr) {
//             return;
//         }
//         // 2d. Null Pointers -> Append literal "<nullptr>"
//         sink.Append("<nullptr>");
//         // 2a. Char Pointers (C strings) -> Append directly
//         if constexpr (detail::is_char_pointer_v<ArgType>) {
//             sink.Append(absl::string_view(value));
//             return;
//         }
//         // 2b. Void Pointers -> Use default pointer formatting (via internal helper/ostream)
//         // Abseil handles %p correctly if the arg is void*. We can provide a default
//         // string representation here if needed for other contexts, like %s.
//         if constexpr (detail::is_void_pointer_v<ArgType>) {
//             // Let's use ostream for a consistent hex representation
//             std::ostringstream oss;
//             oss << value;
//             sink.Append(oss.str());
//             return;
//         }
//         // 2c. Other Non-Null Pointers -> Dereference and Stringify Pointee
//         // Recursively call AbslStringify for the pointee type.
//         AbslStringify(sink, *value);
//         return;
//     }
//     // 3. Handle types with toString()
//     if constexpr (Util::has_toString_v<ArgType>) {
//         // Handle various return types from toString()
//         auto toStringResult = value.toString();
//         using ToStringResultType = std::decay_t<decltype(toStringResult)>;

//         if constexpr (std::is_convertible_v<ToStringResultType, std::string_view>) {
//             // Prefer string_view if convertible (covers std::string, string_view, cstring)
//             sink.Append(std::string_view(toStringResult));
//         } else {
//             // Fallback if toString() returns something else (e.g., int?) - stream it
//             std::ostringstream oss;
//             oss << toStringResult;
//             sink.Append(oss.str());
//         }
//         return;
//     }
//     // 4. Handle types with operator<<
//     if constexpr (requires(std::ostream &os, const ArgType &v) { os << v; }) {
//         std::ostringstream oss;
//         oss << value;
//         sink.Append(oss.str());
//     }
//     throw std::runtime_error(absl::StrCat(
//         "Internal error: Invalid argument type in streamArgument: ", typeid(T).name()));
// }

namespace detail {

inline void getPositionTailLogic(const Util::SourceInfo &info, std::string &position,
                                 std::string &tail) {
    if (!info.isValid()) {
        return;
    }

    cstring posString = info.toPositionString();
    if (position.empty()) {
        position = posString;
    } else if (!posString.isNullOrEmpty()) {
        absl::StrAppend(&tail, "\n", posString);
    }

    cstring fragment = info.toSourceFragment();
    if (!fragment.isNullOrEmpty()) {
        absl::StrAppend(&tail, "\n", fragment);
    }
}

template <typename Tuple, std::size_t... Is>
void extractBugSourceInfoImpl(const Tuple &tup, std::string &position, std::string &tail,
                              std::index_sequence<Is...>) {
    (void)std::initializer_list<int>{(
        [&] {
            const auto &arg = std::get<Is>(tup);
            using ArgType = std::decay_t<decltype(arg)>;

            if constexpr (std::is_same_v<ArgType, P4::Util::SourceInfo>) {
                getPositionTailLogic(arg, position, tail);
            } else if constexpr (Util::has_SourceInfo_v<ArgType> && !std::is_pointer_v<ArgType>) {
                Util::SourceInfo info = arg.getSourceInfo();
                getPositionTailLogic(info, position, tail);
            } else if constexpr (std::is_pointer_v<ArgType>) {
                using PointeeType = std::remove_pointer_t<ArgType>;
                if constexpr (Util::has_SourceInfo_v<PointeeType>) {
                    if (arg != nullptr) {
                        Util::SourceInfo info = arg->getSourceInfo();
                        getPositionTailLogic(info, position, tail);
                    }
                }
            }
        }(),
        0)...};
}

// Optional Helper: Parses a positive integer index after a '%'.
// Returns {index, index_after_digits} on success (index > 0).
// Returns {0, original_start_pos} on failure.
inline std::pair<int, size_t> ParsePlaceholderIndex(std::string_view fmt, size_t start_pos) {
    const char *const begin = fmt.data() + start_pos;
    const char *const end = fmt.data() + fmt.length();
    const char *current = begin;

    // Find the end of the digit sequence
    while (current < end && std::isdigit(static_cast<unsigned char>(*current))) {
        ++current;
    }

    // If no digits were found or went past the end
    if (current == begin) {
        return {0, start_pos};  // Failure: No digits
    }

    int index = 0;
    auto result = std::from_chars(begin, current, index);

    // Check for parsing errors or invalid index (must be > 0 for boost format)
    if (result.ec != std::errc() || result.ptr != current || index <= 0) {
        return {0, start_pos};  // Failure: Parse error or index <= 0
    }

    // Success: return the parsed index and the position *after* the last digit
    return {index, static_cast<size_t>(current - fmt.data())};
}

inline std::string TranslateBoostPlaceholdersToAbsl(std::string_view boost_fmt) {
    std::string result;
    result.reserve(boost_fmt.length() + (boost_fmt.length() / 10) * 2);  // Heuristic reserve

    for (size_t i = 0; i < boost_fmt.length(); ++i) {
        // Common case: Character is not '%'
        if (boost_fmt[i] != '%') {
            result.push_back(boost_fmt[i]);
            continue;
        }

        // We found a '%' at index i. Look ahead.

        // Case 1: '%' is the last character in the string.
        if (i + 1 >= boost_fmt.length()) {
            result.push_back('%');  // Treat as literal '%'
            break;                  // End of string
        }

        // Case 2: Check for '%%' (escaped literal)
        if (boost_fmt[i + 1] == '%') {
            absl::StrAppend(&result, "%%");  // Abseil uses %% for literal %
            i++;                             // Advance past the second '%'
            continue;
        }

        // Case 3: Check for placeholder '%N%'
        if (std::isdigit(static_cast<unsigned char>(boost_fmt[i + 1]))) {
            // Try to parse the number starting after the '%'
            auto [index, end_of_num_idx] = ParsePlaceholderIndex(boost_fmt, i + 1);

            // Check if parsing succeeded AND a closing '%' follows the number
            if (index > 0 && end_of_num_idx < boost_fmt.length() &&
                boost_fmt[end_of_num_idx] == '%') {
                // It's a valid placeholder! Append the Abseil version.
                absl::StrAppend(&result, "%", index, "$v");
                i = end_of_num_idx;  // Advance 'i' past the closing '%'
                continue;
            }
            // If parsing failed OR no closing '%' was found, fall through to Case 4.
        }

        // Case 4: Malformed or not a placeholder (e.g., "%a", "%1", "%"). Treat '%' literally.
        result.push_back('%');
        // The loop increment (++) will handle moving to the next character.
    }

    return result;
}

// This helper function takes an argument. If it's a char* or const char*,
// it returns an absl::string_view representation (handling null).
// Otherwise, it returns the argument unchanged (using perfect forwarding).
template <typename T>
decltype(auto) wrap_char_ptr_arg(T &&arg) {
    // Get the decayed type (removes reference and cv-qualifiers)
    using DecayedT = std::decay_t<T>;

    // Check if the decayed type is specifically char* or const char*
    if constexpr (std::is_same_v<DecayedT, char *> || std::is_same_v<DecayedT, const char *>) {
        // It's a C-style string pointer
        if (arg == nullptr) {
            // Return a specific string_view for null pointers
            return absl::string_view("(null)");
            // Alternative: return absl::string_view{}; // Empty string view
        } else {
            // Create a string_view from the valid C-string pointer
            return absl::string_view(arg);
        }
    } else {
        // Not a char* or const char*, pass it through unchanged
        return std::forward<T>(arg);
    }
}

// Internal formatting function using the translated string and Abseil
template <typename... Args>
std::string StrFormat(std::string_view boost_fmt, Args &&...args) {
    std::string abslFmt = TranslateBoostPlaceholdersToAbsl(boost_fmt);

    // --- FIX: Pass the format string as absl::string_view ---
    // Explicitly construct an absl::string_view from the runtime string.
    // This helps the compiler select the correct overload of absl::StrFormat
    // that expects a string_view rather than the one for FormatSpec.
    std::cout << abslFmt << std::endl;
    auto result = absl::StrFormat(abslFmt.c_str(), wrap_char_ptr_arg(std::forward<Args>(args))...);
    if (result.empty()) {
        throw std::runtime_error(
            absl::StrCat("Error occurred trying to format: \'", abslFmt, "\'"));
    }
    return result;

    // Alternative (less idiomatic for Abseil but might also work if above fails):
}
}  // namespace detail
}  // namespace P4

namespace boost::multiprecision {

// This is the Abseil extension point. It MUST have this signature.
// It will be found via ADL if the type T is in namespace P4, or if it's
// called for a type in another namespace that includes this header.
// If types needing custom formatting are outside P4, you might need to define
// AbslStringify in *their* namespace(s) as well, forwarding to detail::streamArgument.
template <typename Sink, typename T>
void AbslStringify(Sink &sink, const T &value) {
    // Use a stringstream to leverage the existing streamArgument logic
    std::stringstream oss;
    oss << value;
    // Append the stringstream's content to the Abseil sink
    sink.Append(oss.str());
}
}  // namespace boost::multiprecision

namespace P4 {
template <typename Tuple>
void extractBugSourceInfo(const Tuple &tup, std::string &position, std::string &tail) {
    detail::extractBugSourceInfoImpl(tup, position, tail,
                                     std::make_index_sequence<std::tuple_size_v<Tuple>>{});
}

template <typename... Args>
std::string createFormattedMessage(const char *format, Args &&...args) {
    // Ensure string_view is constructed correctly if format can be nullptr
    std::string_view formatSv = format ? std::string_view(format) : std::string_view();
    auto result = detail::StrFormat(formatSv, std::forward<Args>(args)...);
    return result;
}

}  // namespace P4

#endif /* LIB_BOOST_FORMAT_HELPER_H_ */
