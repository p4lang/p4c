#include "backends/p4tools/common/lib/format_bin_hex.h"

#include <stddef.h>

#include <ostream>
#include <string>

#include <boost/multiprecision/cpp_int.hpp>
#include <boost/multiprecision/detail/et_ops.hpp>
#include <boost/multiprecision/traits/explicit_conversion.hpp>

#include "lib/log.h"

namespace P4Tools {

std::string formatBin(const big_int& value, int width, bool useSep, bool pad, bool usePrefix) {
    std::stringstream out;
    if (usePrefix) {
        out << "0b";
    }
    bool haveOutput = false;
    for (; width > 0; width--) {
        // Add an underscore separator if desired.
        bool addSep = useSep && haveOutput && (width % 4 == 0);
        if (addSep) {
            out << "_";
        }

        int shiftAmount = width - 1;
        int curDigit = ((value >> shiftAmount) % 2).convert_to<int>();

        if (curDigit != 0 || pad) {
            out << curDigit;
            haveOutput = true;
        }
    }

    // Ensure we output at least _something_.
    if (!haveOutput) {
        out << 0;
    }

    return out.str();
}

std::string formatOctal(const big_int& value, int width, bool useSep, bool pad, bool usePrefix) {
    std::stringstream out;
    if (usePrefix) {
        out << "0";
    }
    out << std::oct;
    bool haveOutput = false;

    // Widen, if needed.
    width = 2 * ((width + 1) / 2);

    for (; width > 0; width -= 2) {
        // Add an underscore separator if desired.
        bool addSep = useSep && haveOutput && (width % 8 == 0);
        if (addSep) {
            out << "_";
        }

        int shiftAmount = width - 2;
        int curDigit = ((value >> shiftAmount) % 8).convert_to<int>();

        if (curDigit != 0 || pad) {
            out << curDigit;
            haveOutput = true;
        }
    }

    // Ensure we output at least _something_.
    if (!haveOutput) {
        out << 0;
    }
    return out.str();
}

std::string formatHex(const big_int& value, int width, bool useSep, bool pad, bool usePrefix) {
    std::stringstream out;
    if (usePrefix) {
        out << "0x";
    }
    out << std::hex;
    bool haveOutput = false;

    // Widen to a whole nibble.
    width = 4 * ((width + 3) / 4);

    for (; width > 0; width -= 4) {
        // Add an underscore separator if desired.
        bool addSep = useSep && haveOutput && (width % 16 == 0);
        if (addSep) {
            out << "_";
        }

        int shiftAmount = width - 4;
        int curDigit = ((value >> shiftAmount) % 16).convert_to<int>();

        if (curDigit != 0 || pad) {
            out << curDigit;
            haveOutput = true;
        }
    }

    // Ensure we output at least _something_.
    if (!haveOutput) {
        out << 0;
    }
    return out.str();
}

std::string formatBinExpr(const IR::Expression* expr, bool useSep, bool pad, bool usePrefix) {
    if (const auto* constant = expr->to<IR::Constant>()) {
        if (const auto* type = constant->type->to<IR::Type::Bits>()) {
            if (!type->isSigned && type->width_bits() > 0) {
                return formatBin(constant->value, type->width_bits(), useSep, pad, usePrefix);
            }
        }
    }
    // If the precise format does not match, just emit the expression as is.
    // TODO: Make this more precise?
    std::stringstream out;
    out << expr;
    return out.str();
}

std::string formatOctalExpr(const IR::Expression* expr, bool useSep, bool pad, bool usePrefix) {
    if (const auto* constant = expr->to<IR::Constant>()) {
        if (const auto* type = constant->type->to<IR::Type::Bits>()) {
            if (!type->isSigned && type->width_bits() > 0) {
                return formatOctal(constant->value, type->width_bits(), useSep, pad, usePrefix);
            }
        }
    }

    // If the precise format does not match, just emit the expression as is.
    // TODO: Make this more precise?
    std::stringstream out;
    out << expr;
    return out.str();
}

std::string formatHexExpr(const IR::Expression* expr, bool useSep, bool pad, bool usePrefix) {
    if (const auto* constant = expr->to<IR::Constant>()) {
        if (const auto* type = constant->type->to<IR::Type::Bits>()) {
            if (!type->isSigned && type->width_bits() > 0) {
                return formatHex(constant->value, type->width_bits(), useSep, pad, usePrefix);
            }
        }
    }

    // If the precise format does not match, just emit the expression as is.
    // TODO: Make this more precise?
    std::stringstream out;
    out << expr;
    return out.str();
}

std::string formatBinOrHex(const big_int& value, int width, bool useSep, bool pad, bool usePrefix) {
    return width % 4 == 0 ? formatHex(value, width, useSep, pad, usePrefix)
                          : formatBin(value, width, useSep, pad, usePrefix);
}

std::string formatBinOrHexExpr(const IR::Expression* expr, bool useSep, bool pad, bool usePrefix) {
    if (const auto* constant = expr->to<IR::Constant>()) {
        if (const auto* type = constant->type->to<IR::Type::Bits>()) {
            if (!type->isSigned && type->width_bits() > 0) {
                auto value = constant->value;
                auto width = type->width_bits();
                return formatBinOrHex(value, width, useSep, pad, usePrefix);
            }
        }
    }

    std::stringstream out;
    out << expr;
    return out.str();
}

std::string insertOctalSeparators(const std::string& dataStr) {
    std::stringstream formatStr;
    auto stride = 3;
    // Pad the generated string, if the characters do not fit into stride.
    size_t remainder = dataStr.size() % stride;
    if (remainder != 0) {
        size_t pad = stride - remainder;
        formatStr << std::string(pad, '0') << dataStr.substr(0, remainder);
    }
    // Insert a separator every stride.
    for (size_t dataPos = remainder; dataPos < dataStr.size() - 1; dataPos += stride) {
        formatStr << "\\" << dataStr.substr(dataPos, stride);
    }
    return formatStr.str();
}

std::string insertHexSeparators(const std::string& dataStr) {
    std::stringstream formatStr;
    auto stride = 2;
    // Pad the generated string, if the characters do not fit into stride.
    size_t remainder = dataStr.size() % stride;
    if (remainder != 0) {
        size_t pad = stride - remainder;
        formatStr << std::string(pad, '0') << dataStr.substr(0, remainder);
    }
    // Insert a separator every stride.
    for (size_t dataPos = remainder; dataPos < dataStr.size() - 1; dataPos += stride) {
        formatStr << "\\x" << dataStr.substr(dataPos, stride);
    }
    return formatStr.str();
}

}  // namespace P4Tools
