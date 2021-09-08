#ifndef BACKENDS_TC_PASS_MANAGER_H_
#define BACKENDS_TC_PASS_MANAGER_H_

#include <memory>
#include <sstream>

#include "absl/container/flat_hash_map.h"
#include "absl/types/any.h"
#include "backends/tc/pass.h"
#include "backends/tc/tcam_program.h"
#include "lib/error.h"
#include "lib/exceptions.h"

// Pass manager that runs registered transformation passes over a TCAM program,
// and any analysis result that a transformation pass needs.

namespace backends::tc {

// Pass manager that keeps track of versioned TCAM programs, and runs given
// transformation passes. It automatically runs any needed analysis pass, caches
// analysis results, and backtracks if a transformation or analysis pass fails.
class PassManager {
 public:
  PassManager() {}

  // Register given transform pass.
  void AddPass(std::unique_ptr<TransformPass>);

  // Run the registered passes on the given program
  absl::optional<TCAMProgram> RunPasses(const TCAMProgram& input_program);

  // Return the result of the last successful pass
  const TCAMProgram* LatestSuccessfulPassResult() const {
    if (tcam_programs_.empty()) {
      return nullptr;
    }
    return &tcam_programs_.back();
  }

  // Get the cached result of a given analysis, if the result is not available,
  // run the analysis.
  template <class P>
  const typename P::Result* GetAnalysisResult() {
    if (!analysis_results_.contains(P::kName)) {
      // The result is not cached, run the analysis and save the result
      active_passes_.push_back(P::kName);
      P analysis(*this);
      auto result = analysis.Run(tcam_programs_.back());
      if (result.ok()) {
        analysis_results_.emplace(P::kName, result.value());
      } else {
        // The analysis has failed
        analysis_results_.emplace(P::kName, absl::nullopt);
        std::ostringstream active_pass_list;
        for (const auto& pass : active_passes_) {
          active_pass_list << pass << "\n";
        }
        ::error(
            "The analysis pass %s has failed. All active passes (in the order "
            "of calling) are\n%s",
            P::kName, active_pass_list.str());
      }
      active_passes_.pop_back();
    }
    auto& maybe_result = analysis_results_.at(P::kName);
    if (!maybe_result) {
      // The analysis has previously failed
      return nullptr;
    }
    const typename P::Result* result =
        absl::any_cast<typename P::Result>(&*maybe_result);
    BUG_CHECK(
        result,
        "Found the result of a different analysis when looking up the result "
        "of %s. Maybe two analysis passes are registered under the same name?",
        P::kName);
    return result;
  }

 private:
  // The transform passes to run
  std::vector<std::unique_ptr<TransformPass>> transform_passes_;
  // The stack of TCAM programs for backtracking
  std::vector<TCAMProgram> tcam_programs_;
  // The analysis results for the most-recent TCAM program. If an analysis has
  // failed, then this mapping contains `absl::nullopt` for that analysis.
  absl::flat_hash_map<absl::string_view, absl::optional<absl::any>>
      analysis_results_;
  // The passes that are being run, this is used for debugging to print all the
  // clients of a pass that was being run.
  std::vector<absl::string_view> active_passes_;
};

}  // namespace backends::tc

#endif  // BACKENDS_TC_PASS_MANAGER_H_
