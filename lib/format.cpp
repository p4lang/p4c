// SPDX-FileCopyrightText: 2026 The P4 Language Consortium
//
// SPDX-License-Identifier: Apache-2.0

#include "lib/format.h"

#include <algorithm>
#include <charconv>
#include <cstdint>
#include <iomanip>

#include "absl/strings/str_cat.h"
#include "lib/exceptions.h"

namespace P4::FormatDetail {
namespace {

bool isDigit(char c) { return c >= '0' && c <= '9'; }

[[noreturn]] void invalidFormat(std::string_view format, std::string_view reason) {
    throw Util::CompilerBug(
        absl::StrCat("Invalid diagnostic format '", format, "': ", reason, "\n"));
}

class FormatNormalizer {
    enum class Indexing : std::uint8_t { Unset, Numbered, Sequential };

    std::string_view format;
    size_t argumentCount;
    size_t cursor = 0;
    size_t nextArgument = 1;
    size_t maxArgument = 0;
    Indexing indexing = Indexing::Unset;

    [[nodiscard]] char peek() const { return cursor < format.size() ? format[cursor] : '\0'; }

    std::string_view consumeDigits() {
        size_t start = cursor;
        while (isDigit(peek())) {
            ++cursor;
        }
        return format.substr(start, cursor - start);
    }

    size_t argumentIndex(std::string_view number) {
        auto mode = number.empty() ? Indexing::Sequential : Indexing::Numbered;
        if (indexing != Indexing::Unset && indexing != mode) {
            invalidFormat(format, "cannot mix numbered and unnumbered arguments");
        }
        indexing = mode;
        size_t index = 0;
        if (number.empty()) {
            index = nextArgument++;
        } else {
            auto parsed = std::from_chars(number.data(), number.data() + number.size(), index);
            if (parsed.ec != std::errc()) {
                invalidFormat(format, "argument index overflow");
            }
        }
        if (index == 0 || index > argumentCount) {
            invalidFormat(format, "argument index out of range");
        }
        maxArgument = std::max(maxArgument, index);
        return index;
    }

    std::string consumeStar() {
        ++cursor;
        auto number = consumeDigits();
        if (!number.empty()) {
            if (peek() != '$') {
                invalidFormat(format, "expected $ after star argument index");
            }
            ++cursor;
        }
        return absl::StrCat("*", argumentIndex(number), "$");
    }

    bool consumeLength() {
        // Abseil infers the argument's size, so printf length modifiers are redundant.
        if (peek() == '\0' || std::string_view("hljztL").find(peek()) == std::string_view::npos) {
            return false;
        }
        char length = format[cursor++];
        if ((length == 'h' || length == 'l') && peek() == length) {
            ++cursor;
        }
        return true;
    }

    char consumeConversion(bool bracketed) {
        const bool hasLength = consumeLength();
        char conversion = peek();
        if (bracketed && conversion == '|' && !hasLength) {
            conversion = 's';
        } else {
            if (conversion == '\0' ||
                std::string_view("scdiuoxXfFeEgGaApv").find(conversion) == std::string_view::npos) {
                invalidFormat(format, "unsupported or incomplete conversion");
            }
            ++cursor;
        }
        if (bracketed) {
            if (peek() != '|') {
                invalidFormat(format, "expected closing |");
            }
            ++cursor;
        }
        return conversion;
    }

    std::string consumePlaceholder() {
        bool bracketed = peek() == '|';
        if (bracketed) {
            ++cursor;
        }
        size_t start = cursor;
        auto number = consumeDigits();
        if (!bracketed && !number.empty() && peek() == '%') {
            ++cursor;
            return absl::StrCat("%", argumentIndex(number) + argumentCount, "$s");
        }
        size_t index = 0;
        if (!number.empty() && peek() == '$') {
            index = argumentIndex(number);
            ++cursor;
        } else {
            cursor = start;  // Those digits describe a width, not an argument.
        }
        std::string flags;
        while (peek() != '\0' && std::string_view("-+ #0").find(peek()) != std::string_view::npos) {
            flags += format[cursor++];
        }
        std::string width = peek() == '*' ? consumeStar() : std::string(consumeDigits());
        std::string precision;
        if (peek() == '.') {
            ++cursor;
            precision = ".";
            precision += peek() == '*' ? consumeStar() : std::string(consumeDigits());
        }
        const char conversion = consumeConversion(bracketed);
        if (index == 0) {
            index = argumentIndex({});
        }
        if (conversion == 's') {
            index += argumentCount;
        }
        return absl::StrCat("%", index, "$", flags, width, precision,
                            std::string_view(&conversion, 1));
    }

 public:
    FormatNormalizer(std::string_view format, size_t argumentCount)
        : format(format), argumentCount(argumentCount) {}

    std::string normalize() {
        std::string result;
        while (cursor < format.size()) {
            if (format[cursor++] != '%') {
                result += format[cursor - 1];
            } else if (peek() == '%') {
                result += "%%";
                ++cursor;
            } else {
                result += consumePlaceholder();
            }
        }
        if (maxArgument != argumentCount) {
            invalidFormat(format, "too many arguments");
        }
        return result;
    }
};

}  // namespace

std::string normalizeFormat(std::string_view format, size_t argumentCount) {
    return FormatNormalizer(format, argumentCount).normalize();
}

std::string formatMessage(std::string_view format, absl::Span<const absl::FormatArg> arguments) {
    auto normalized = normalizeFormat(format, arguments.size() / 2);
    std::string result;
    if (!absl::FormatUntyped(&result, absl::UntypedFormatSpec(normalized), arguments)) {
        invalidFormat(format, "conversion does not match argument type");
    }
    return result;
}

bool appendStreamArgument(std::string text, const absl::FormatConversionSpec &spec,
                          absl::FormatSink *sink, bool arithmetic) {
    using Char = absl::FormatConversionChar;
    int precision = spec.conversion_char() == Char::s ? spec.precision() : -1;
    if (spec.conversion_char() == Char::c) {
        precision = 1;
    }
    if (precision >= 0 && text.size() > static_cast<size_t>(precision)) {
        text.resize(precision);
    }
    if (spec.has_sign_col_flag() && !spec.has_show_pos_flag() &&
        (text.empty() || (text.front() != '-' && text.front() != '+'))) {
        text.insert(0, 1, ' ');
    }
    if (spec.has_zero_flag() && !spec.has_left_flag() && spec.width() > 0 &&
        text.size() < static_cast<size_t>(spec.width())) {
        size_t padding = spec.width() - text.size();
        // Built-in numbers place padding after the sign. Other streamed values,
        // including big_int, place padding before the entire text.
        if (arithmetic && !text.empty() &&
            (text.front() == '-' || text.front() == '+' || text.front() == ' ')) {
            sink->Append(std::string_view(text).substr(0, 1));
            text.erase(0, 1);
        }
        sink->Append(padding, '0');
        sink->Append(text);
        return true;
    }
    return sink->PutPaddedString(text, spec.width(), -1, spec.has_left_flag());
}

void setFormatFlags(std::ostream &out, const absl::FormatConversionSpec &spec) {
    using Char = absl::FormatConversionChar;
    switch (spec.conversion_char()) {
        case Char::X:
            out << std::uppercase;
            [[fallthrough]];
        case Char::x:
            out << std::hex;
            break;
        case Char::o:
            out << std::oct;
            break;
        case Char::F:
            out << std::uppercase;
            [[fallthrough]];
        case Char::f:
            out << std::fixed;
            break;
        case Char::E:
            out << std::uppercase;
            [[fallthrough]];
        case Char::e:
            out << std::scientific;
            break;
        case Char::G:
            out << std::uppercase;
            break;
        case Char::A:
            out << std::uppercase;
            [[fallthrough]];
        case Char::a:
            out << std::hexfloat;
            break;
        case Char::c:
        case Char::s:
        case Char::d:
        case Char::i:
        case Char::u:
        case Char::g:
        case Char::p:
        case Char::v:
            // Text, decimal integers, and general floats use the fresh stream's defaults.
            break;
        case Char::n:
            invalidFormat("%n", "write-count conversion is not supported");
    }
    if (spec.has_show_pos_flag()) {
        out << std::showpos;
    }
    if (spec.has_alt_flag()) {
        out << std::showbase << std::showpoint;
    }
    if (spec.precision() >= 0 && spec.conversion_char() != Char::s) {
        out << std::setprecision(spec.precision());
    }
}

}  // namespace P4::FormatDetail
