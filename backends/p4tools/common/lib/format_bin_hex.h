#ifndef COMMON_LIB_FORMAT_BIN_HEX_H_
#define COMMON_LIB_FORMAT_BIN_HEX_H_

#include <string>

#include "ir/ir.h"
#include "lib/gmputil.h"

namespace P4Tools {

/// Format @arg value as either binary or hexadecimal string.
/// Hexadecimal strings are prefixed with "0x", whereas binary strings are prefixed with "0b". If
/// @arg useSep is true, the output is formatted in groups of four digits separated by the
/// underscore character. If not a multiple of four, the first group will have the remainder
/// digits; for example, "0xBAD_BEEF" for hex or "0b10_1010" for binary.
///
/// If @arg pad is true, then the output will be padded on the left with zeroes to visually
/// represent the full width of the expression's type.
/// If @arg usePrefix is true, the string will contain a prefix denoting the numeral system.
std::string formatBinOrHex(const big_int& value, int width, bool useSep = false, bool pad = true,
                           bool usePrefix = true);

/// Format @arg value as a binary string.
/// @see formatBinOrHex
std::string formatBin(const big_int& value, int width, bool useSep = false, bool pad = true,
                      bool usePrefix = true);

/// Format @arg value as a octal string.
/// @see formatBinOrHex
std::string formatOctal(const big_int& value, int width, bool useSep = false, bool pad = true,
                        bool usePrefix = true);

/// Format @arg value as a hexadecimal string.
/// @see formatBinOrHex
std::string formatHex(const big_int& value, int width, bool useSep = false, bool pad = true,
                      bool usePrefix = true);

/// If the given @arg expr is an unsigned bit-typed constant, it is formatted as a binary or
/// hexadecimal string. Otherwise, this delegates to P4C's operator<<.
///
/// @see formatBinOrHex
std::string formatBinOrHexExpr(const IR::Expression* expr, bool useSep = false, bool pad = true,
                               bool usePrefix = true);

/// If the given @arg expr is an unsigned bit-typed constant, it is formatted as a binary string.
/// Otherwise, this delegates to P4C's operator<<.
///
/// @see formatBinOrHexExpr.
std::string formatBinExpr(const IR::Expression* expr, bool useSep = false, bool pad = true,
                          bool usePrefix = true);

/// If the given @arg expr is an unsigned bit-typed constant, it is formatted as an octal
/// string. Otherwise, this delegates to P4C's operator<<.
///
/// @see formatBinOrHexExpr.
std::string formatOctalExpr(const IR::Expression* expr, bool useSep = false, bool pad = true,
                            bool usePrefix = true);

/// If the given @arg expr is an unsigned bit-typed constant, it is formatted as a hexadecimal
/// string. Otherwise, this delegates to P4C's operator<<.
///
/// @see formatBinOrHexExpr.
std::string formatHexExpr(const IR::Expression* expr, bool useSep = false, bool pad = true,
                          bool usePrefix = true);

/// Takes an octal-formatted string as input and inserts slashes as separators.
std::string insertOctalSeparators(const std::string& dataStr);

/// Takes a hex-formatted string as input and inserts slashes as separators.
std::string insertHexSeparators(const std::string& dataStr);

}  // namespace P4Tools

#endif /* COMMON_LIB_FORMAT_BIN_HEX_H_ */
