#ifndef BACKENDS_TC_REACHABILITY_H_
#define BACKENDS_TC_REACHABILITY_H_

#include "absl/container/flat_hash_map.h"
#include "absl/container/flat_hash_set.h"
#include "backends/tc/pass_manager.h"

namespace backends::tc {

// The result of the reachability analysis.
struct ReachabilityResult {
  // The call graph, it is a mapping from each state to all potential next
  // states. We keep a separate call graph because this is a more efficient
  // representation than the call graph representation the p4c frontend uses.
  absl::flat_hash_map<State, absl::flat_hash_set<State>> call_graph;

  // Transitive closure of the call graph, it is a mapping from each state to
  // all states it can reach.
  absl::flat_hash_map<State, absl::flat_hash_set<State>> transitive_closure;
};

// Reachability analysis for parser states
class Reachability final
    : public AnalysisPass<Reachability, ReachabilityResult> {
 public:
  constexpr static const char* kName = "Reachability";
  using Result = ReachabilityResult;
  absl::StatusOr<Result> Run(const TCAMProgram& tcam_program) override;

  explicit Reachability(PassManager& pass_manager)
      : AnalysisPass(pass_manager) {}
};

}  // namespace backends::tc

#endif  // BACKENDS_TC_REACHABILITY_H_
