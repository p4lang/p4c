// SPDX-FileCopyrightText: 2026 The P4 Language Consortium
//
// SPDX-License-Identifier: Apache-2.0

#ifndef LIB_FORMAT_H_
#define LIB_FORMAT_H_

#include <array>
#include <functional>
#include <ostream>
#include <sstream>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>

#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"
#include "absl/types/span.h"
#include "lib/source_file.h"
#include "lib/stringify.h"

namespace P4 {
namespace FormatDetail {

// Normalize supported placeholders to positional Abseil conversions. Arguments
// have two representations: native values followed by their stream adapters.
// %N% and %s use the stream adapters for an argument's text representation.
std::string normalizeFormat(std::string_view format, size_t argumentCount);
std::string formatMessage(std::string_view format, absl::Span<const absl::FormatArg> arguments);
void setFormatFlags(std::ostream &out, const absl::FormatConversionSpec &spec);
bool appendStreamArgument(std::string text, const absl::FormatConversionSpec &spec,
                          absl::FormatSink *sink, bool arithmetic);

template <bool PreferDbprint, typename T>
void streamArgument(std::ostream &out, const T &arg) {
    // Keep arrays distinct from pointers: string literals cannot be null.
    using Type = std::remove_cvref_t<T>;
    if constexpr (std::is_same_v<Type, Util::SourceInfo>) {
        // A source location consumes a placeholder but contributes no text.
    } else if constexpr (std::is_same_v<Type, std::nullptr_t>) {
        out << "<nullptr>";
    } else if constexpr (std::is_pointer_v<Type>) {
        using Pointee = std::remove_pointer_t<Type>;
        if (arg == nullptr) {
            out << "<nullptr>";
        } else if constexpr (std::is_same_v<std::remove_cv_t<Pointee>, char> ||
                             std::is_void_v<Pointee> || std::is_function_v<Pointee>) {
            out << arg;
        } else if constexpr (std::is_same_v<std::remove_cv_t<Pointee>, Util::SourceInfo>) {
            // SourceInfo pointers follow the same convention as references.
        } else if constexpr (PreferDbprint) {
            // Bug reports use pointer insertion unless dbprint exists.
            if constexpr (has_dbprint_v<Pointee>) {
                arg->dbprint(out);
            } else {
                out << arg;
            }
        } else {
            streamArgument<PreferDbprint>(out, *arg);
        }
    } else if constexpr (PreferDbprint && has_dbprint_v<Type>) {
        arg.dbprint(out);
    } else if constexpr (!PreferDbprint && Util::has_toString_v<Type>) {
        out << arg.toString();
    } else {
        static_assert(has_ostream_operator_v<T>,
                      "cannot format this type; implement toString, dbprint, or operator<<");
        out << arg;
    }
}

// Keep references: diagnostic arguments can be noncopyable IR nodes, and
// FormatArg itself does not own the value to which it refers.
template <bool PreferDbprint, typename T>
struct StreamArgument {
    std::reference_wrapper<const T> value;

    friend absl::FormatConvertResult<
        absl::FormatConversionCharSet::kString | absl::FormatConversionCharSet::kNumeric |
        absl::FormatConversionCharSet::c | absl::FormatConversionCharSet::v |
        absl::FormatConversionCharSet::p>
    // Abseil requires this customization-point name.
    // NOLINTNEXTLINE(readability-identifier-naming)
    AbslFormatConvert(const StreamArgument &arg, const absl::FormatConversionSpec &spec,
                      absl::FormatSink *sink) {
        std::ostringstream out;
        setFormatFlags(out, spec);
        if constexpr (std::is_pointer_v<T> && !std::is_function_v<std::remove_pointer_t<T>>) {
            if (spec.conversion_char() == absl::FormatConversionChar::p) {
                out << static_cast<const void *>(arg.value.get());
            } else {
                streamArgument<PreferDbprint>(out, arg.value.get());
            }
        } else {
            streamArgument<PreferDbprint>(out, arg.value.get());
        }
        return {appendStreamArgument(out.str(), spec, sink, std::is_arithmetic_v<T>)};
    }
};

template <bool PreferDbprint, typename T>
absl::FormatArg nativeArgument(const StreamArgument<PreferDbprint, T> &arg) {
    if constexpr (std::is_arithmetic_v<T>) {
        return absl::FormatArg(arg.value.get());
    } else {
        return absl::FormatArg(arg);
    }
}

template <bool PreferDbprint, typename Tuple, size_t... Is>
std::string formatTuple(std::string_view format, const Tuple &args,
                        [[maybe_unused]] std::index_sequence<Is...> indices) {
    if constexpr (sizeof...(Is) == 0) {
        // Still parse the format to handle %% and reject missing arguments.
        return formatMessage(format, {});
    } else {
        auto adapters = std::tuple{
            StreamArgument<PreferDbprint, std::remove_cvref_t<decltype(std::get<Is>(args))>>{
                std::get<Is>(args)}...};
        std::array<absl::FormatArg, 2 * sizeof...(Is)> arguments{
            nativeArgument(std::get<Is>(adapters))..., absl::FormatArg(std::get<Is>(adapters))...};
        return formatMessage(format, arguments);
    }
}

// Use the same source-info traversal for errors and bugs, including pointers
// to SourceInfo. Unrelated arguments must not erase previously found locations.
template <typename T, typename Emit>
void visitSourceInfo(const T &arg, const Emit &emit) {
    using Type = std::remove_cvref_t<T>;
    if constexpr (std::is_same_v<Type, Util::SourceInfo>) {
        if (arg.isValid()) {
            emit(arg);
        }
    } else if constexpr (std::is_pointer_v<Type>) {
        if constexpr (!std::is_void_v<std::remove_pointer_t<Type>> &&
                      !std::is_function_v<std::remove_pointer_t<Type>>) {
            if (arg != nullptr) {
                visitSourceInfo(*arg, emit);
            }
        }
    } else if constexpr (Util::has_SourceInfo_v<Type>) {
        visitSourceInfo(arg.getSourceInfo(), emit);
    }
}

}  // namespace FormatDetail

/// Format runtime diagnostic strings using Abseil. Supports %N%, %N$spec,
/// %|N$spec|, and printf conversions. Numbered and unnumbered arguments cannot
/// be mixed. Invalid formats/counts throw Util::CompilerBug, as formatting
/// errors are compiler bugs. Boost-only tabulation and centering are unsupported.
/// Explicit numeric conversions use Abseil's type rules (e.g. %d requires an
/// integer); %N% and %s use the argument's text representation for any type.
template <bool PreferDbprint = false, typename Tuple>
std::string createFormattedMessageFromTuple(const char *format, const Tuple &args) {
    return FormatDetail::formatTuple<PreferDbprint>(
        format, args, std::make_index_sequence<std::tuple_size_v<Tuple>>{});
}

template <bool PreferDbprint = false, typename... Args>
std::string createFormattedMessage(const char *format, const Args &...args) {
    return createFormattedMessageFromTuple<PreferDbprint>(format, std::tie(args...));
}

template <typename... Args>
std::string createBugMessage(const char *format, const Args &...args) {
    std::string position;
    std::string tail;
    if constexpr (sizeof...(Args) != 0) {
        auto collect = [&](const Util::SourceInfo &info) {
            auto pos = info.toPositionString();
            if (position.empty()) {
                position = pos;
            } else if (!pos.isNullOrEmpty()) {
                absl::StrAppend(&tail, pos, "\n");
            }
            absl::StrAppend(&tail, info.toSourceFragment());
        };
        (FormatDetail::visitSourceInfo(args, collect), ...);
    }
    return absl::StrCat(position, position.empty() ? "" : ": ",
                        createFormattedMessage<true>(format, args...), "\n", tail);
}

}  // namespace P4

#endif /* LIB_FORMAT_H_ */
