#include "backends/tc/inlining.h"

#include <algorithm>
#include <deque>
#include <optional>

#include "absl/container/flat_hash_map.h"
#include "backends/tc/instruction.h"
#include "backends/tc/reachability.h"
#include "backends/tc/tcam_program.h"

namespace backends::tc {

namespace {
// Computes the topological ordering of states reachable from the start state.
// This helper is used for finding the optimal order to visit the states.
std::vector<State> TopologicallySortStates(
    const ReachabilityResult& reachability) {
  size_t next_ordinal = 0;
  absl::flat_hash_map<State, size_t> topological_ordinals;
  std::deque<State> worklist;
  worklist.emplace_back(kStartState);
  // Compute the topological order using breadth-first search with cycle
  // detection.
  while (!worklist.empty()) {
    auto current = worklist.front();
    if (!topological_ordinals.contains(current)) {
      topological_ordinals.emplace(State(current), next_ordinal++);
      auto callees = reachability.call_graph.find(current);
      if (callees != reachability.call_graph.end()) {
        for (const auto& callee : callees->second) {
          worklist.emplace_back(callee);
        }
      }
    }
    worklist.pop_front();
  }

  std::vector<State> states;
  // Reserve the space so we can use emplace_back to insert the strings without
  // maintaining an index.
  states.reserve(topological_ordinals.size());
  for (const auto& state : topological_ordinals) {
    states.emplace_back(state.first);
  }
  std::sort(states.begin(), states.end(),
            [&](const State& state1, const State& state2) {
              return topological_ordinals.at(state1) <
                     topological_ordinals.at(state2);
            });
  return states;
}

}  // namespace

absl::StatusOr<TCAMProgram> Inlining::Run(const TCAMProgram& tcam_program) {
  // The inlining pass works in the following phases:
  //
  // 1. Compute a topological ordering of states.
  // 2. Visit the states in reverse topological ordering, and inline the next
  // state. This guarantees that any inlining done in the current state will be
  // propagated to transitive callers, and inlining is done in only one pass
  // through the states.

  // Topological ordering
  auto reachability = pass_manager_.GetAnalysisResult<Reachability>();
  if (!reachability) {
    return absl::InternalError("Failed to get the reachability results");
  }

  auto sorted_states = TopologicallySortStates(*reachability);
  TCAMProgram new_program(tcam_program);
  // Visit the states in reverse
  for (auto state = sorted_states.crbegin(), end = sorted_states.crend();
       state != end; ++state) {
    // Skip states with no entries such as accept and reject
    if (!new_program.tcam_table_.contains(*state)) {
      continue;
    }
    for (auto& entry : new_program.tcam_table_.at(*state)) {
      // Traverse the instructions to find:
      // 1. The next state, so we can fetch the instructions
      // 2. The total move offset, so we can adjust the ranges in the
      // instructions of the next state
      absl::optional<State> next_state;
      size_t move_offset = 0;
      for (const auto& instruction : entry.instructions) {
        if (auto next = dynamic_cast<const NextState*>(&*instruction)) {
          next_state = next->state();
        } else if (auto move = dynamic_cast<const Move*>(&*instruction)) {
          move_offset += move->offset();
        }
      }

      // Do not inline if there isn't a next state, or if next state is special
      if (!next_state || IsSpecial(*next_state)) {
        continue;
      }
      auto next_state_entries = new_program.tcam_table_.find(*next_state);
      // Whether given bitstring consists of all zeros
      auto all_zeros = [](const std::vector<bool>& bitstring) {
        return std::find(bitstring.begin(), bitstring.end(), true) ==
               bitstring.end();
      };
      // Whether this state always transitions to a fixed, known entry. This is
      // the case when there is only one entry for the next state and its mask
      // is all 0s so there are no implicit transitions to "reject".
      bool transition_to_known_entry =
          next_state_entries != new_program.tcam_table_.end() &&
          next_state_entries->second.size() == 1 &&
          all_zeros(next_state_entries->second.at(0).mask);
      // Remove the NextState instructions if we are going to perform inlining.
      if (next_state_entries == new_program.tcam_table_.end() ||
          next_state_entries->second.empty() || transition_to_known_entry) {
        auto new_end = std::remove_if(
            entry.instructions.begin(), entry.instructions.end(),
            [](const std::shared_ptr<const Instruction>& instruction) -> bool {
              return dynamic_cast<const NextState*>(&*instruction);
            });
        // Shrink the vector to remove the leftover entries at the end after
        // removal
        entry.instructions.resize(new_end - entry.instructions.begin());
      }
      // Copy over the instructions from next_state's instruction list if there
      // is one, while adjusting offsets if necessary
      if (next_state_entries != new_program.tcam_table_.end() &&
          next_state_entries->second.size() == 1) {
        auto& instructions_to_copy =
            next_state_entries->second.front().instructions;
        for (auto& instruction : instructions_to_copy) {
          if (move_offset) {
            // Inspect the current instruction see if there is any offset
            // adjustment is needed
            if (auto set_key = dynamic_cast<const SetKey*>(&*instruction)) {
              entry.instructions.push_back(std::make_shared<SetKey>(
                  set_key->range().OffsetBy(move_offset)));
            } else if (auto store = dynamic_cast<const StoreHeaderField*>(
                           &*instruction)) {
              entry.instructions.push_back(std::make_shared<StoreHeaderField>(
                  store->range().OffsetBy(move_offset), store->header_field()));
            } else {
              entry.instructions.push_back(instruction);
            }
          } else {
            // The current instruction table does not move the cursor at all, so
            // we can just copy the instructions
            entry.instructions.push_back(instruction);
          }
        }
      }
    }
  }

  return new_program;
}

}  // namespace backends::tc
