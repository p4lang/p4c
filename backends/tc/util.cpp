#include "backends/tc/util.h"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <cstdlib>
#include <sstream>
#include <stdexcept>

#include "absl/status/status.h"
#include "absl/strings/match.h"
#include "absl/strings/str_split.h"
#include "absl/strings/string_view.h"
#include "lib/exceptions.h"

namespace {

const char kHexDigitTable[16] = {'0', '1', '2', '3', '4', '5', '6', '7',
                                 '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

}

namespace backends::tc::util {

absl::StatusOr<Range> Range::Create(uint32_t from, uint32_t to) {
  if (from > to) {
    return absl::InvalidArgumentError(
        "Start offset of range is larger than the end offset");
  }
  return Range(from, to);
}

absl::StatusOr<Range> Range::Parse(absl::string_view string) {
  std::vector<absl::string_view> components = absl::StrSplit(string, "..");
  if (components.size() != 2) {
    return absl::InvalidArgumentError(
        "Range to parse must be of the form from..to, the given range contains "
        "more than one instance of '..'");
  }
  auto from = ParseUInt32(components.at(0));
  if (!from.ok()) {
    return from.status();
  }
  auto to = ParseUInt32(components.at(1));
  if (!to.ok()) {
    return to.status();
  }
  return Range::Create(*from, *to);
}

std::ostream& operator<<(std::ostream& out, const Range& range) {
  out << range.from() << ".." << range.to();
  return out;
}

void PrettyPrintBitString(const std::vector<bool>& bit_string,
                          std::ostream& output) {
  auto length = bit_string.size();

  if (length == 0) {
    output << "0w0";
    return;
  }

  output << length << "w0x";

  auto iter = bit_string.begin();
  auto end = bit_string.end();

  auto append_bit = [](size_t& digit, bool bit) { digit = digit * 2 + bit; };

  // The first hexadecimal digit may need to represent less than 4 bits, so
  // extract bits until the remaining length is divisible by 4 and print the
  // first digit.
  if (length % 4 != 0) {
    size_t first_digit = 0;
    for (; length % 4 != 0 && iter != end; ++iter, --length) {
      first_digit = 2 * first_digit + *iter;
    }

    output << kHexDigitTable[first_digit];
  }

  // Now the length must be divisible by 4, read 4 bits and output a digit until
  // reaching the end.
  while (iter != end) {
    size_t digit = 0;
    // read and append 4 bits, MSB-first
    for (int i = 0; i < 4; ++i) {
      append_bit(digit, *iter++);
    }

    output << kHexDigitTable[digit];
  }
}

std::string BitStringToString(const std::vector<bool>& bit_string) {
  std::ostringstream out;
  PrettyPrintBitString(bit_string, out);
  return out.str();
}

std::vector<bool> UInt64ToBitString(uint64_t n) {
  std::vector<bool> result;

  // special handling for 0, so that we do not return an empty bit string
  if (n == 0) {
    result.push_back(false);
    return result;
  }

  // construct the vector lsb-first, then reverse
  while (n > 0) {
    result.push_back(n % 2 != 0);
    n /= 2;
  }

  std::reverse(result.begin(), result.end());

  return result;
}

std::vector<bool> UInt64ToBitString(uint64_t n, size_t size) {
  // First, convert n to a bit-string then handle the size mismatches.
  auto bit_string = UInt64ToBitString(n);
  if (bit_string.size() > size) {
    std::vector<bool> result(size, false);
    std::copy_n(bit_string.rbegin(), size, result.rbegin());
    return result;
  } else if (bit_string.size() < size) {
    std::vector<bool> result(size, false);
    std::copy(bit_string.rbegin(), bit_string.rend(), result.rbegin());
    return result;
  } else {
    return bit_string;
  }
}

std::vector<bool> HexStringToBitString(size_t size,
                                       absl::string_view hex_string) {
  std::vector<bool> bit_string(size, false);
  if (size == 0) {
    return bit_string;
  }

  // Go through the string right-to-left, so that we fill the vector LSB-first
  // and pad it right. We also keep track of the index we are accessing.
  ssize_t i = size - 1;
  for (auto orig_hex_char = hex_string.rbegin();
       orig_hex_char != hex_string.rend(); orig_hex_char++) {
    char hex_char = std::tolower(*orig_hex_char);
    BUG_CHECK(
        ('0' <= hex_char && hex_char <= '9') ||
            ('a' <= hex_char && hex_char <= 'f'),
        "Expected a string of hexadecimal digits, found '%c' in string '%s'",
        std::string(1, *orig_hex_char), std::string(hex_string));

    int hex_digit;

    if ('0' <= hex_char && hex_char <= '9') {
      hex_digit = hex_char - '0';
    } else {
      hex_digit = hex_char - 'a' + 10;
    }

    for (int j = 0; j < 4; ++j, --i) {
      bit_string[i] = hex_digit & (1 << j);
      if (i == 0) {
        // Stop if hex_string is too long
        return bit_string;
      }
    }
  }

  return bit_string;
}

absl::StatusOr<std::vector<bool>> SizedStringToBitString(
    absl::string_view string_with_size) {
  std::vector<absl::string_view> size_and_value =
      absl::StrSplit(string_with_size, 'w');
  if (size_and_value.size() != 2) {
    return absl::InvalidArgumentError(
        "The input is not a valid number prefixed by size");
  }
  auto size = ParseUInt32(size_and_value.at(0));
  if (!size.ok()) {
    return size.status();
  }
  auto& value = size_and_value.at(1);
  if (absl::StartsWith(value, "0x")) {
    // Interpret the value as a hexadecimal string
    return HexStringToBitString(*size, value.substr(2));
  } else if (std::all_of(value.begin(), value.end(),
                         [](char c) { return c == '0'; })) {
    // Handle the case where all digits are zero for now.
    // Support parsing all decimal strings as well in the future.
    return UInt64ToBitString(0, *size);
  } else {
    return absl::UnimplementedError(
        "Parsing non-hexadecimal bitstrings is not implemented yet.");
  }
}

absl::StatusOr<uint32_t> ParseUInt32(absl::string_view string) {
  // Check that the input consists of only decimal digits
  if (!std::all_of(string.begin(), string.end(),
                   [](char c) { return c >= '0' && c <= '9'; })) {
    return absl::InvalidArgumentError(
        "The string to parse contains characters other than decimal digits.");
  }

  // Assuming that `long` is 32 bits. We can use `std::from_chars` once P4C
  // switches to C++17.
  size_t processed_len;
  uint32_t result;
  try {
    result = std::stoul(std::string(string), &processed_len, 10);
  } catch (std::invalid_argument&) {
    return absl::InvalidArgumentError(
        "The string to parse does not represent a valid integer.");
  } catch (std::out_of_range&) {
    return absl::OutOfRangeError(
        "The number encoded in the string cannot be represented in 32 bits.");
  }
  if (processed_len != string.size()) {
    return absl::InvalidArgumentError(
        "The string to parse does not represent a valid integer.");
  }
  return result;
}

}  // namespace backends::tc::util
