#ifndef BACKENDS_TC_TEST_UTIL_H_
#define BACKENDS_TC_TEST_UTIL_H_

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

}  // namespace backends::tc

#endif  // BACKENDS_TC_TEST_UTIL_H_
