#ifndef BACKENDS_P4TOOLS_COMMON_LIB_FORMAT_INT_H_
#define BACKENDS_P4TOOLS_COMMON_LIB_FORMAT_INT_H_

#include <stddef.h>

#include <string>

#include "ir/ir.h"
#include "lib/big_int_util.h"

namespace P4Tools {

/// Defines common formatting options.
struct FormatOptions {
    /// Insert separators into the produced string.
    /// The output is formatted in groups of four digits separated by the underscore character.
    /// If not a multiple of four, the first group will have the remainder
    ///  digits; for example, "0xBAD_BEEF" for hex or "0b10_1010" for binary.
    bool useSeparator = false;

    /// The output will be padded on the left with zeroes to
    /// visually represent the full requested width.
    bool padOutput = true;

    /// The string will contain a prefix denoting the numeral system.
    /// Hexadecimal strings are prefixed with "0x",
    /// whereas binary strings are prefixed with "0b" and octal strings with "0".
    bool usePrefix = true;

    /// In case of hex strings, use uppercase instead of lowercase letters.
    bool useUpperCaseHex = true;
};

/// Format @arg value as either binary or hexadecimal string.
std::string formatBinOrHex(const big_int &value,
                           const FormatOptions &formatOptions = FormatOptions());

/// Format @arg value as a binary string.
std::string formatBin(const big_int &value, int width,
                      const FormatOptions &formatOptions = FormatOptions());

/// Format @arg value as a octal string.
std::string formatOctal(const big_int &value, const FormatOptions &formatOptions = FormatOptions());

/// Format @arg value as a hexadecimal string.
std::string formatHex(const big_int &value, int width,
                      const FormatOptions &formatOptions = FormatOptions());

/// If the given @arg expr is an unsigned bit-typed constant, it is formatted as a binary or
/// hexadecimal string. Otherwise, this delegates to P4C's operator<<.
///
std::string formatBinOrHexExpr(const IR::Expression *expr,
                               const FormatOptions &formatOptions = FormatOptions());

/// If the given @arg expr is an unsigned bit-typed constant, it is formatted as a binary string.
/// Otherwise, this delegates to P4C's operator<<.
///
std::string formatBinExpr(const IR::Expression *expr,
                          const FormatOptions &formatOptions = FormatOptions());

/// If the given @arg expr is an unsigned bit-typed constant, it is formatted as an octal
/// string. Otherwise, this delegates to P4C's operator<<.
///
std::string formatOctalExpr(const IR::Expression *expr,
                            const FormatOptions &formatOptions = FormatOptions());

/// If the given @arg expr is an unsigned bit-typed constant, it is formatted as a hexadecimal
/// string. Otherwise, this delegates to P4C's operator<<.
///
std::string formatHexExpr(const IR::Expression *expr,
                          const FormatOptions &formatOptions = FormatOptions());

/// Insert separators into @param dataStr every @param stride. If @param skipFirst is true, this
/// will not add a separator to the beginning of the string. This function always pads to the width
/// of @param stride.
std::string insertSeparators(const std::string &dataStr, const std::string &separator = "\\x",
                             size_t stride = 2, bool skipFirst = false);

/// Takes an octal-formatted string as input and inserts slashes as separators.
std::string insertOctalSeparators(const std::string &dataStr);

/// Takes a hex-formatted string as input and inserts slashes as separators.
std::string insertHexSeparators(const std::string &dataStr);

/// Converts a big integer input into a vector of bytes with the size targetWidthBits /8.
/// If the integer value is smaller than the request bit size and @param padLeft is false, padding
/// is added on the right-hand side of the vector (the "least-significant" side). Otherwise, padding
/// is added on the left-hand side, (the "most-significant side")
std::vector<uint8_t> convertBigIntToBytes(const big_int &dataInt, int targetWidthBits,
                                          bool padLeft = false);

/// Convert a byte array into a IPv4 string of the form "x.x.x.x".
/// @returns std::nullopt if the conversion fails.
std::optional<std::string> convertToIpv4String(const std::vector<uint8_t> &byteArray);

/// Convert a byte array into a IPv4 string of the form "xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:xxxx".
/// @returns std::nullopt if the conversion fails.
std::optional<std::string> convertToIpv6String(const std::vector<uint8_t> &byteArray);

/// Convert a byte array into a MAC string of the form "xx:xx:xx:xx:xx:xx".
/// @returns std::nullopt if the conversion fails.
std::optional<std::string> convertToMacString(const std::vector<uint8_t> &byteArray);

}  // namespace P4Tools

#endif /* BACKENDS_P4TOOLS_COMMON_LIB_FORMAT_INT_H_ */
