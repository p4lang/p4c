#ifndef BACKENDS_TC_UTIL_H_
#define BACKENDS_TC_UTIL_H_

#include <ostream>
#include <vector>

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/string_view.h"

namespace backends::tc::util {

// A range that consists of two offsets
class Range {
 public:
  static absl::StatusOr<Range> Create(uint32_t from, uint32_t to);
  inline uint32_t from() const { return this->from_; }
  inline uint32_t to() const { return this->to_; }
  inline uint32_t size() const { return to() - from(); }

  Range(const Range&) = default;
  Range(Range&&) = default;
  Range& operator=(const Range&) = default;

  bool operator==(const Range& that) const {
    return this->from() == that.from() && this->to() == that.to();
  }

  // Compute a new range that is offset by the given amount from this range.
  Range OffsetBy(int32_t offset) const {
    return Range(from() + offset, to() + offset);
  }

  // Parse the range represented as from..to
  static absl::StatusOr<Range> Parse(absl::string_view string);

 private:
  Range(uint32_t from, uint32_t to) : from_(from), to_(to) {}
  uint32_t from_;
  uint32_t to_;
};

std::ostream& operator<<(std::ostream& out, const Range& range);

// Pretty prints the given bit string (MSB-first) in the format
// "<length>w0x<hex>" where,
//
// - <length> is the length of the bitstring
// - <hex> is the hexadecimal representation of the contents
//
// For example, the bitvector 110 would be printed as '3w0x6'.
void PrettyPrintBitString(const std::vector<bool>& bit_string,
                          std::ostream& output);

// String representation of given bit string according to how it is printed by
// `PrettyPrintBitString`.
std::string BitStringToString(const std::vector<bool>& bit_string);

// Helper function to convert an unsigned int to bitstring, msb-first
std::vector<bool> UInt64ToBitString(uint64_t n);

// Helper function to convert an unsigned int to bitstring with given size,
// msb-first. If n is too large to fit in `size` bits, then the most-significant
// bits are discarded. If n requires fewer bits than `size` to represent, then
// it is padded right, and the most-significant bits are set to 0. This behavior
// is same as the behavior of HexStringToBitString.
std::vector<bool> UInt64ToBitString(uint64_t n, size_t size);

// Helper function to convert a hexadecimal string to bitstring, msb-first. if
// the hex string is too long, then the most-significant bits are discarded. If
// it is too short, then it is padded right (used for the least-significant
// bits) and most-significant bits are set to zero.
std::vector<bool> HexStringToBitString(size_t size,
                                       absl::string_view hex_string);

// Convert a sized string to bit string. This function expects a string of the
// form SIZEwVALUE where VALUE is a hexadecimal string
// prefixed by 0x. Support for decimal representation could be added in the
// future.
//
// Its padding behavior is same as that of `HexStringToBitString`.
absl::StatusOr<std::vector<bool>> SizedStringToBitString(
    absl::string_view string_with_size);

// Parse given string as a number. This method is a more flexible wrapper around
// the standard library methods for parsing integers, and it returns
// `absl::StatusOr` instead of an error code or throwing an exception.
absl::StatusOr<uint32_t> ParseUInt32(absl::string_view string);

// A function object for `operator<<` where left-hand side is a stream. The
// standard library does not provide a named functional object for `operator<<`.
template <class Lhs, class Rhs>
struct WriteToStream {
  auto operator()(Lhs&& lhs, Rhs&& rhs) const
      -> decltype(std::forward<Lhs>(lhs) << std::forward<Rhs>(rhs)) {
    return std::forward<Lhs>(lhs) << std::forward<Rhs>(rhs);
  }
};

template <class Lhs>
struct WriteToStream<Lhs, void> {};

template <class Rhs>
struct WriteToStream<void, Rhs> {};

template <>
struct WriteToStream<void, void> {};

// A higher-order function that implements fold (reduce) left on the type
// level. This is a substitute for fold expressions in C++17.
//
// The binary operator BinaryOp is assumed to have the type signature
// `template<typename First, typename Second> First(First, Second)` modulo
// lvalue/rvalue forwarding. That is, it should produce a functional object
// class implements a binary operator of type `(First * Second) -> First`
template <template <class, class> class BinaryOp, class Arg>
Arg FoldLeft(Arg&& arg) {
  // Base case 1, forward the argument
  return std::forward<Arg>(arg);
}
template <template <class, class> class BinaryOp, class First, class Second>
auto FoldLeft(First&& first, Second&& second)
    -> decltype(std::forward<First>(first)) {
  // Base case 2, apply the operation only once
  return BinaryOp<First, Second>{}(std::forward<First>(first),
                                   std::forward<Second>(second));
}
template <template <class, class> class BinaryOp, class First, class Second,
          class... Rest>
auto FoldLeft(First&& first, Second&& second, Rest&&... rest)
    -> decltype(std::forward<First>(first)) {
  // Recursive case
  return FoldLeft<BinaryOp>(
      BinaryOp<First, Second>{}(std::forward<First>(first),
                                std::forward<Second>(second)),
      std::forward<Rest>(rest)...);
}

}  // namespace backends::tc::util

#endif  // BACKENDS_TC_UTIL_H_
