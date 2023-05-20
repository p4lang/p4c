#include "backends/tc/test_util.h"

#include "absl/memory/memory.h"

// Utility functions shared among tests

namespace backends::tc {

TCAMProgram TCAMProgramWithGivenTransitions(
    const absl::flat_hash_map<State, absl::btree_set<State>>& transitions) {
  TCAMTable tcam_table;

  // The strategy we follow to generate the keys is as follows:
  //
  // For any given state, if there is only one element then the value and the
  // mask are both empty (0-bit) values. If there are multiple transitions from
  // a state, we generate one TCAM entry per transition:
  //
  // 1. The first entry we generate is the default one, so it's value and mask
  // are both 16w0x0000 (16-bit values with all 0s).
  // 2. All other entries have the mask 16w0xFFFF, and they each get a value
  // based on the iteration order of the btree_set.
  //
  // We use the scheme above to generate representatives for both in-states that
  // have empty masks and values, and out-states that have non-empty masks and
  // values easily.
  for (const auto& transition : transitions) {
    const auto& from_state = transition.first;
    const auto& to_states = transition.second;
    // TCAM entries for `from_state`
    auto& tcam_entries =
        tcam_table.emplace(from_state, std::vector<TCAMEntry>{}).first->second;

    if (to_states.size() == 1) {
      tcam_entries.push_back(TCAMEntry{
          .value = {},
          .mask = {},
          .instructions =
              {
                  absl::make_unique<NextState>(*to_states.begin()),
                  absl::make_unique<SetKey>(util::Range::Create(0, 16).value()),
                  absl::make_unique<Move>(16),
              },
      });
    } else {
      uint16_t value = 0;
      for (const auto& to_state : to_states) {
        uint16_t mask = (value == 0) ? 0x0000 : 0xFFFF;

        tcam_entries.push_back(TCAMEntry{
            .value = util::UInt64ToBitString(value, 16),
            .mask = util::UInt64ToBitString(mask, 16),
            .instructions =
                {
                    absl::make_unique<NextState>(to_state),
                    absl::make_unique<SetKey>(
                        util::Range::Create(0, 16).value()),
                    absl::make_unique<Move>(16),
                },
        });
        value++;
      }
    }
  }

  return TCAMProgram{std::move(tcam_table), {}, {}};
}

TCAMProgram TCAMProgramWithTestHeaders(TCAMTable&& tcam_table) {
  return TCAMProgram{
      .tcam_table_ = std::move(tcam_table),
      .header_instantiations_ = {{"foo", "foo_t"}},
      .header_type_definitions_ = {{"foo_t", {{"bar", 8}, {"baz", 8}}}},
  };
}

}  // namespace backends::tc
