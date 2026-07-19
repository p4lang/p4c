#ifndef LIB_BOOST_FORMAT_HELPER_H_
#define LIB_BOOST_FORMAT_HELPER_H_

#include <cctype>
#include <charconv>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <tuple>
#include <type_traits>
#include <typeinfo>
#include <utility>
#include <vector>

#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"
#include "lib/cstring.h"
#include "lib/source_file.h"
#include "lib/stringify.h"

namespace P4 {

namespace detail {
// Formatter adapter for legacy boost::format placeholders (%N%) and
// printf/absl::StrFormat placeholders (%s, %d, ...).
// The style is auto-detected; mixed styles are rejected.

template <typename T>
constexpr bool is_char_pointer_v = std::is_same_v<T, char *> || std::is_same_v<T, const char *>;

template <typename T>
void streamArgument(std::ostream &os, const T &arg) {
    // Use decay_t to handle potential references correctly in type checks
    using ArgType = std::decay_t<T>;

    if constexpr (std::is_same_v<ArgType, P4::Util::SourceInfo>) {
        // Do nothing - source info is collected elsewhere.
        return;
    }

    if constexpr (std::is_pointer_v<ArgType>) {
        if (arg != nullptr) {
            if constexpr (is_char_pointer_v<ArgType>) {
                os << arg;
            } else {
                streamArgument(os, *arg);
            }
        } else {
            os << "<nullptr>";
        }
    } else if constexpr (Util::has_toString_v<ArgType>) {
        os << arg.toString();
    } else if constexpr (requires { os << arg; }) {
        os << arg;
    } else {
        throw std::runtime_error(absl::StrCat(
            "Internal error: Invalid argument type in streamArgument: ", typeid(ArgType).name()));
    }
}

// Modify streamNthImpl to call the smart helper
template <typename Tuple, std::size_t... Is>
void streamNthImpl(std::ostream &os, std::size_t index, const Tuple &tup,
                   std::index_sequence<Is...>) {
    bool found = ((index == Is && (streamArgument(os, std::get<Is>(tup)), true)) || ...);
    if (!found) {
        throw std::runtime_error(absl::StrCat(
            "Internal error: Invalid argument index requested in streamNthImpl: ", index));
    }
}

// streamNth wrapper remains the same
template <typename Tuple>
void streamNth(std::ostream &os, std::size_t index, const Tuple &tup) {
    streamNthImpl(os, index, tup, std::make_index_sequence<std::tuple_size_v<Tuple>>{});
}

// Replicates the logic from the original getPositionTail, modifying args by reference
inline void getPositionTailLogic(const Util::SourceInfo &info, std::string &position,
                                 std::string &tail) {
    if (!info.isValid()) {
        return;
    }

    cstring posString = info.toPositionString();
    if (position.empty()) {
        position = posString;
    } else if (!posString.isNullOrEmpty()) {
        // Append position only if subsequent one is found and non-empty
        absl::StrAppend(&tail, "\n", posString);
    }

    cstring fragment = info.toSourceFragment();
    if (!fragment.isNullOrEmpty()) {
        absl::StrAppend(&tail, "\n", fragment);
    }
}

/// Iterates through tuple elements, extracts SourceInfo, updates position/tail.
template <typename Tuple, std::size_t... Is>
void extractBugSourceInfoImpl(const Tuple &tup, std::string &position, std::string &tail,
                              std::index_sequence<Is...>) {
    // Use initializer list and lambda with constexpr if to process each element
    (void)std::initializer_list<int>{
        (
            [&] {
                const auto &arg = std::get<Is>(tup);
                using ArgType = std::decay_t<decltype(arg)>;

                // Is the argument itself SourceInfo?
                if constexpr (std::is_same_v<ArgType, P4::Util::SourceInfo>) {
                    getPositionTailLogic(arg, position, tail);
                } else if constexpr (Util::has_SourceInfo_v<ArgType> &&
                                     !std::is_pointer_v<ArgType>) {
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
            0)...  // 0 is placeholder for initializer list expansion
    };
}

enum class FormatStyle { Unknown, Boost, Abseil, Literal };

inline constexpr FormatStyle detectFormatStyle(std::string_view fmt) {
    bool hasBoost = false;
    bool hasAbseil = false;

    for (size_t i = 0; i < fmt.size(); ++i) {
        if (fmt[i] == '%') {
            if (i + 1 >= fmt.size()) {
                // Lone % at end - consider it Abseil style
                hasAbseil = true;
                if (hasBoost) return FormatStyle::Unknown;  // Mixed
                break;                                      // Nothing more to parse
            }

            char nextChar = fmt[i + 1];

            if (nextChar == '%') {  // Escaped %%
                i++;                // Skip the second %
                continue;
            }

            // Check specifically for Boost style: % followed by digits followed by %
            if (std::isdigit(static_cast<unsigned char>(nextChar))) {
                size_t j = i + 1;  // Start of digits
                while (j < fmt.size() && std::isdigit(static_cast<unsigned char>(fmt[j]))) {
                    j++;
                }
                // Check if it ends *exactly* with '%'
                if (j < fmt.size() && fmt[j] == '%') {
                    // Found %N% pattern
                    hasBoost = true;
                    if (hasAbseil) return FormatStyle::Unknown;  // Mixed style detected
                    i = j;                                       // Move index past the closing '%'
                    continue;  // Proceed to next part of the string
                }
                // If it was digits but didn't end in '%', it's Abseil style
                // Let it fall through to the general Abseil case below.
            }

            // If it wasn't %% and wasn't a valid %N% sequence,
            // treat it as the start of an Abseil style specifier.
            // We don't need to parse the rest of the Abseil specifier for detection.
            hasAbseil = true;
            if (hasBoost) return FormatStyle::Unknown;  // Mixed style detected

            // We can simply skip the next character (% followed by non-digit/non-%),
            // as the outer loop will increment 'i'. More complex skipping isn't needed
            // just for *detection*. The actual formatting function handles parsing.
            // i++; // Implicitly skipped by outer loop increment
        }
        // Ignore non-% characters
    }

    // Final classification based on exclusively found styles
    if (hasBoost && hasAbseil) return FormatStyle::Unknown;
    if (hasBoost) return FormatStyle::Boost;
    if (hasAbseil) return FormatStyle::Abseil;
    return FormatStyle::Literal;
}

template <typename Tuple>
std::string formatBoostStyle(std::string_view formatSv, const Tuple &argTuple) {
    std::stringstream ss;
    constexpr std::size_t numArgs = std::tuple_size_v<Tuple>;
    std::size_t currentPos = 0;

    while (currentPos < formatSv.size()) {
        size_t nextPercent = formatSv.find('%', currentPos);
        if (nextPercent == std::string_view::npos) {
            ss << formatSv.substr(currentPos);
            break;
        }
        ss << formatSv.substr(currentPos, nextPercent - currentPos);
        currentPos = nextPercent;

        if (currentPos + 1 >= formatSv.size()) {
            ss << '%';
            currentPos++;
            continue;
        }

        if (formatSv[currentPos + 1] == '%') {  // %%
            ss << '%';
            currentPos += 2;
        } else if (std::isdigit(static_cast<unsigned char>(formatSv[currentPos + 1]))) {  // %N%
            size_t j = currentPos + 1;
            while (j < formatSv.size() && std::isdigit(static_cast<unsigned char>(formatSv[j]))) {
                ++j;
            }
            if (j < formatSv.size() && formatSv[j] == '%') {
                std::string_view numSv = formatSv.substr(currentPos + 1, j - (currentPos + 1));
                uint64_t argIndexBoost = 0;
                auto result =
                    std::from_chars(numSv.data(), numSv.data() + numSv.size(), argIndexBoost);
                if (result.ec == std::errc() && argIndexBoost > 0 && argIndexBoost <= numArgs) {
                    detail::streamNth(ss, static_cast<std::size_t>(argIndexBoost - 1), argTuple);
                    currentPos = j + 1;
                } else {
                    ss << '%';
                    currentPos++;
                }  // Invalid N
            } else {
                ss << '%';
                currentPos++;
                // %digits without %
            }
        } else {
            // % followed by non-% non-digit
            ss << '%';
            currentPos++;
        }
    }
    return ss.str();
}

template <typename Tuple, std::size_t... Is>  // Is is the pack 0, 1, 2...
void populateSelectiveAbseilArgsImpl(const Tuple &tup, std::vector<std::string> &argStrings,
                                     std::vector<absl::FormatArg> &abslArgs,
                                     std::index_sequence<Is...>) {
    abslArgs.reserve(std::tuple_size_v<Tuple>);
    argStrings.reserve(std::tuple_size_v<Tuple>);

    // Use initializer list and lambda with constexpr if to process each element.
    (void)std::initializer_list<int>{
        (
            [&] {
                const auto &element = std::get<Is>(tup);
                using ElementType = std::decay_t<decltype(element)>;

                if constexpr (std::is_integral_v<ElementType>) {
                    // Pass integral natively.
                    abslArgs.push_back(absl::FormatArg(element));
                } else {
                    // Convert other types to string.
                    std::stringstream tempSs;
                    detail::streamArgument(tempSs, element);
                    argStrings.push_back(tempSs.str());
                    abslArgs.push_back(absl::FormatArg(argStrings.back()));
                }
            }(),
            0)...  // 0 is placeholder for initializer list expansion
    };
}

// Wrapper to generate the index sequence
template <typename Tuple>
void populateSelectiveAbseilArgs(const Tuple &tup, std::vector<std::string> &argStrings,
                                 std::vector<absl::FormatArg> &abslArgs) {
    populateSelectiveAbseilArgsImpl(tup, argStrings, abslArgs,
                                    std::make_index_sequence<std::tuple_size_v<Tuple>>{});
}

template <typename Tuple>
std::string formatAbslStyle(std::string_view formatSv, const Tuple &argTuple) {
    // 1. Populate abslArgs vector: integrals passed directly, others as strings
    std::vector<std::string> argStrings;
    std::vector<absl::FormatArg> abslArgs;
    populateSelectiveAbseilArgs(argTuple, argStrings, abslArgs);

    // 2. Call Abseil Format
    std::string result;
    absl::UntypedFormatSpec spec(formatSv);
    if (!absl::FormatUntyped(&result, spec, abslArgs)) {
        // Formatting might fail if specifier doesn't match type.
        // e.g., %d applied to a pre-rendered string or the "" placeholder.
        // Or %s applied to an integral (Abseil might handle this, though).
        throw std::runtime_error(
            absl::StrCat("absl::Format error. Can not format with the provided arguments"));
    }
    return result;
}

}  // namespace detail

template <typename Tuple>
void extractBugSourceInfo(const Tuple &tup, std::string &position, std::string &tail) {
    detail::extractBugSourceInfoImpl(tup, position, tail,
                                     std::make_index_sequence<std::tuple_size_v<Tuple>>{});
}

template <typename Tuple>
std::string createFormattedMessageFromTuple(const char *format, const Tuple &argTuple) {
    std::string_view formatSv(format);
    detail::FormatStyle style = detail::detectFormatStyle(formatSv);

    try {
        switch (style) {
            case detail::FormatStyle::Boost:
                return detail::formatBoostStyle(formatSv, argTuple);
            case detail::FormatStyle::Abseil:
                // Pass the original tuple to the Abseil formatter.
                return detail::formatAbslStyle(formatSv, argTuple);
            case detail::FormatStyle::Literal:
                return std::string(formatSv);
            // Mixed or invalid style
            case detail::FormatStyle::Unknown:
            default:
                throw std::runtime_error("Can not determine appropriate format.");
        }
    } catch (const std::exception &e) {
        return absl::StrCat("<<Internal Formatting Error: ", e.what(), " | Format: ", format, ">>");
    } catch (...) {
        return absl::StrCat("<<Unknown Internal Formatting Error", " | Format: ", format, ">>");
    }
}

template <typename... Args>
std::string createFormattedMessage(const char *format, Args &&...args) {
    auto argTuple = std::make_tuple(std::forward<Args>(args)...);
    return createFormattedMessageFromTuple(format, argTuple);
}

}  // namespace P4

#endif /* LIB_BOOST_FORMAT_HELPER_H_ */
