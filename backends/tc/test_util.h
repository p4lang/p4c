#ifndef BACKENDS_TC_TEST_UTIL_H_
#define BACKENDS_TC_TEST_UTIL_H_

#include <cstdint>

#include "absl/container/btree_set.h"
#include "backends/tc/tcam_program.h"

// Utility functions shared among tests

namespace backends::tc {

// Helper function that creates a TCAM program from given state transitions.
//
// If B is in transitions[A], then the generated TCAM program has an entry for A
// that contains the instruction "next-state B" along with potentially other
// instructions. The generated TCAM program has an arbitrary value and mask for
// all entries. The value and the mask are stable across calls, the function
// uses `absl::btree_set`.
TCAMProgram TCAMProgramWithGivenTransitions(
    const absl::flat_hash_map<State, absl::btree_set<State>>& transitions);

// Helper function that converts a raw byte (char) iterator to a bitstring.
//
// `IteratorT` must implement LegacyRandomAccessIterator
template <class IteratorT>
std::vector<bool> ToBitString(IteratorT begin, IteratorT end) {
  std::vector<bool> result;
  // pre-allocate the vector
  result.reserve(end - begin);
  for (auto byte = begin; byte != end; ++byte) {
    // push the bits in MSB-first order
    for (int i = 7; i >= 0; --i) {
      result.push_back((*byte) & (1 << i));
    }
  }

  return result;
}

// Helper function to read/write bytes in network (big endian) byte order. It
// reads/writes the least significant `size` bytes.
template <class T>
void WriteBytes(const T& value, size_t size, std::vector<uint8_t>& output) {
  for (; size > 0; --size) {
    auto shift_amount = 8 * (size - 1);
    output.push_back((value & (T(0xFF) << shift_amount)) >> shift_amount);
  }
}

// Helper function to build the TCAM program with the relevant headers used in
// some tests.
TCAMProgram TCAMProgramWithTestHeaders(TCAMTable&& tcam_table);

}  // namespace backends::tc

#endif  // BACKENDS_TC_TEST_UTIL_H_
