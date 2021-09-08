#include "backends/tc/reachability.h"

#include "absl/container/flat_hash_set.h"
#include "backends/tc/instruction.h"
#include "frontends/p4/callGraph.h"

namespace backends::tc {

// Definition needed for linking in pre-C++17
constexpr const char* Reachability::kName;

absl::StatusOr<Reachability::Result> Reachability::Run(
    const TCAMProgram& tcam_program) {
  Result reachability_result;
  // Use the call graph analysis from p4c's mid-end to compute reachability
  P4::CallGraph<State> call_graph("TCAMStateReachability");
  // List of all states that are in the TCAM table
  std::vector<State> states;
  // Build the call graph
  for (const auto& state_to_entries : tcam_program.tcam_table_) {
    auto& from_state = state_to_entries.first;
    auto& tcam_entries = state_to_entries.second;
    states.push_back(from_state);
    auto& callees =
        reachability_result.call_graph
            .emplace(State(from_state), absl::flat_hash_set<State>{})
            .first->second;
    for (const auto& tcam_entry : tcam_entries) {
      for (const auto& instruction : tcam_entry.instructions) {
        if (const auto next_state =
                dynamic_cast<const NextState*>(&*instruction)) {
          // Insert the call to both representations.
          call_graph.calls(from_state, next_state->state());
          callees.insert(next_state->state());
        }
      }
    }
  }
  // Compute reachability for all states
  for (const State& state : states) {
    // Reachable states computed by the p4c call graph analysis. It uses
    // `std::set` which is slower than `absl::flat_hash_set`, so we use this
    // variable as a temporary and move its contents to `reachable_states`.
    std::set<State> call_graph_output;
    call_graph.reachable(state, call_graph_output);
    // Reachable states this analysis will return
    auto& reachable_states =
        reachability_result.transitive_closure
            .emplace(State(state), absl::flat_hash_set<State>{})
            .first->second;
    for (auto&& s : call_graph_output) {
      reachable_states.emplace(s);
    }
  }
  return reachability_result;
}

}  // namespace backends::tc
