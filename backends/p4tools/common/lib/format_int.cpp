#include "backends/p4tools/common/lib/format_int.h"

#include <algorithm>
#include <iomanip>
#include <ostream>
#include <string>

#include <boost/multiprecision/cpp_int.hpp>
#include <boost/multiprecision/cpp_int/bitwise.hpp>
#include <boost/multiprecision/detail/et_ops.hpp>
#include <boost/multiprecision/number.hpp>
#include <boost/multiprecision/traits/explicit_conversion.hpp>

#include "lib/cstring.h"
#include "lib/exceptions.h"

namespace P4Tools {

std::string formatBin(const big_int &value, int width, bool useSep, bool pad, bool usePrefix) {
    std::stringstream out;
    // Ensure we output at least _something_.
    if (width == 0) {
        if (usePrefix) {
            out << "0b";
        }
        out << 0;
        return out.str();
    }

    BUG_CHECK(value >= 0, "Negative values not supported for binary formatting.");

    // Use a custom procedure to convert big_int to a binary sequence.
    // Lifted from https://stackoverflow.com/a/2891232.
    auto tmpVal = value;
    do {
        out << static_cast<int>(boost::multiprecision::bit_test(tmpVal, 0));
    } while (tmpVal >>= 1);

    if (pad && width > out.tellp()) {
        out << std::string(width - out.tellp(), '0');
    }
    auto returnString = out.str();
    // Use big-endian ordering.
    std::reverse(returnString.begin(), returnString.end());

    if (useSep) {
        returnString = insertSeparators(returnString, "_", 4, true);
    }

    if (usePrefix) {
        returnString.insert(0, "0b");
    }
    return returnString;
}

std::string formatOctal(const big_int &value, int width, bool useSep, bool pad, bool usePrefix) {
    std::stringstream out;
    // Ensure we output at least _something_.
    if (width == 0) {
        if (usePrefix) {
            out << "0";
        }
        out << 0;
        return out.str();
    }

    BUG_CHECK(value >= 0, "Negative values not supported for octal formatting.");

    // Use oct printing format.
    out << std::oct;

    // Widen to 4 bit.
    width = ((width + 1) / 2);

    if (pad) {
        out << std::setfill('0') << std::setw(width);
    }
    out << value;
    auto returnString = out.str();
    if (useSep) {
        returnString = insertSeparators(returnString, "_", 4, true);
    }

    if (usePrefix) {
        returnString.insert(0, "0");
    }

    return returnString;
}

std::string formatHex(const big_int &value, int width, bool useSep, bool pad, bool usePrefix) {
    std::stringstream out;
    // Ensure we output at least _something_.
    if (width == 0) {
        if (usePrefix) {
            out << "0x";
        }
        out << 0;
        return out.str();
    }

    BUG_CHECK(value >= 0, "Negative values not supported for hex formatting.");

    // Use hex printing format. Use uppercase format.
    out << std::hex << std::uppercase;

    // Widen to a whole nibble.
    width = ((width + 3) / 4);

    if (pad) {
        out << std::setfill('0') << std::setw(width);
    }
    out << value;
    auto returnString = out.str();
    if (useSep) {
        returnString = insertSeparators(returnString, "_", 4, true);
    }

    if (usePrefix) {
        returnString.insert(0, "0x");
    }

    return returnString;
}

std::string formatBinExpr(const IR::Expression *expr, bool useSep, bool pad, bool usePrefix) {
    if (const auto *constant = expr->to<IR::Constant>()) {
        auto val = constant->value;
        if (const auto *type = constant->type->to<IR::Type::Bits>()) {
            if (type->isSigned && val < 0) {
                // Invert a negative value by subtracting it from the maximum possible value
                // respective to the width.
                auto limit = big_int(1);
                limit <<= type->width_bits();
                val = limit + val;
            }
            return formatBin(val, type->width_bits(), useSep, pad, usePrefix);
        }
    }

    if (const auto *boolExpr = expr->to<IR::BoolLiteral>()) {
        std::stringstream out;
        if (usePrefix) {
            out << "0b";
        }
        out << boolExpr->value;
        return out.str();
    }

    if (const auto *stringExpr = expr->to<IR::StringLiteral>()) {
        // TODO: Include the quotes here?
        return stringExpr->value.c_str();
    }

    P4C_UNIMPLEMENTED("Binary formatting for expression %1% of type %2% not supported.", expr,
                      expr->type);
}

std::string formatOctalExpr(const IR::Expression *expr, bool useSep, bool pad, bool usePrefix) {
    if (const auto *constant = expr->to<IR::Constant>()) {
        auto val = constant->value;
        if (const auto *type = constant->type->to<IR::Type::Bits>()) {
            if (type->isSigned && val < 0) {
                // Invert a negative value by subtracting it from the maximum possible value
                // respective to the width.
                auto limit = big_int(1);
                limit <<= type->width_bits();
                val = limit + val;
            }
            return formatOctal(val, type->width_bits(), useSep, pad, usePrefix);
        }
    }

    if (const auto *boolExpr = expr->to<IR::BoolLiteral>()) {
        std::stringstream out;
        if (usePrefix) {
            out << "0";
        }
        out << boolExpr->value;
        return out.str();
    }

    if (const auto *stringExpr = expr->to<IR::StringLiteral>()) {
        // TODO: Include the quotes here?
        return stringExpr->value.c_str();
    }

    P4C_UNIMPLEMENTED("Octal formatting for expression %1% of type %2% not supported.", expr,
                      expr->type);
}

std::string formatHexExpr(const IR::Expression *expr, bool useSep, bool pad, bool usePrefix) {
    if (const auto *constant = expr->to<IR::Constant>()) {
        auto val = constant->value;
        if (const auto *type = constant->type->to<IR::Type::Bits>()) {
            if (type->isSigned && val < 0) {
                // Invert a negative value by subtracting it from the maximum possible value
                // respective to the width.
                auto limit = big_int(1);
                limit <<= type->width_bits();
                val = limit + val;
            }
            return formatHex(val, type->width_bits(), useSep, pad, usePrefix);
        }
    }

    if (const auto *boolExpr = expr->to<IR::BoolLiteral>()) {
        std::stringstream out;
        if (usePrefix) {
            out << "0x";
        }
        out << boolExpr->value;
        return out.str();
    }

    if (const auto *stringExpr = expr->to<IR::StringLiteral>()) {
        // TODO: Include the quotes here?
        return stringExpr->value.c_str();
    }

    P4C_UNIMPLEMENTED("Hexadecimal formatting for expression %1% of type %2% not supported.", expr,
                      expr->type);
}

std::string formatBinOrHex(const big_int &value, int width, bool useSep, bool pad, bool usePrefix) {
    return width % 4 == 0 ? formatHex(value, width, useSep, pad, usePrefix)
                          : formatBin(value, width, useSep, pad, usePrefix);
}

std::string formatBinOrHexExpr(const IR::Expression *expr, bool useSep, bool pad, bool usePrefix) {
    return expr->type->width_bits() % 4 == 0 ? formatHexExpr(expr, useSep, pad, usePrefix)
                                             : formatBinExpr(expr, useSep, pad, usePrefix);
}

std::string insertSeparators(const std::string &dataStr, const std::string &separator,
                             size_t stride, bool skipFirst) {
    size_t stringWidth = dataStr.size();
    // Nothing to do if we skip the first character and the stride is as wide as the string itself.
    if (stringWidth <= stride && skipFirst) {
        return dataStr;
    }
    std::stringstream formatStr;
    // Pad the generated string, if the characters do not fit into stride.
    size_t remainder = stringWidth % stride;
    if (!skipFirst) {
        formatStr << separator;
    }
    size_t dataPos = remainder;
    if (remainder != 0) {
        size_t pad = stride - remainder;
        formatStr << std::string(pad, '0') << dataStr.substr(0, remainder);
    } else {
        formatStr << dataStr.substr(0, stride);
        dataPos = stride;
    }
    // Insert a separator every stride.
    for (; dataPos < dataStr.size() - 1; dataPos += stride) {
        formatStr << separator << dataStr.substr(dataPos, stride);
    }
    return formatStr.str();
}

std::string insertOctalSeparators(const std::string &dataStr) {
    return insertSeparators(dataStr, "\\", 3, false);
}

std::string insertHexSeparators(const std::string &dataStr) {
    return insertSeparators(dataStr, "\\x", 2, false);
}

}  // namespace P4Tools
