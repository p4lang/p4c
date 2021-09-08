#include "backends/tc/dead_state_elimination.h"

#include "backends/tc/reachability.h"

namespace backends::tc {

absl::StatusOr<TCAMProgram> DeadStateElimination::Run(
    const TCAMProgram& tcam_program) {
  if (const auto reachability =
          pass_manager_.GetAnalysisResult<Reachability>()) {
    TCAMProgram result(tcam_program);
    const auto& reachable_states =
        reachability->transitive_closure.at(kStartState);
    for (const auto& state_to_entries : tcam_program.tcam_table_) {
      auto& state = state_to_entries.first;
      if (!reachable_states.contains(state)) {
        result.tcam_table_.erase(state);
      }
    }
    return result;
  } else {
    return absl::InternalError("Could not get reachability results");
  }
}

}  // namespace backends::tc
