#include "backends/tc/pass_manager.h"

#include <optional>

namespace backends::tc {

void PassManager::AddPass(std::unique_ptr<TransformPass> pass) {
  transform_passes_.push_back(std::move(pass));
}

absl::optional<TCAMProgram> PassManager::RunPasses(
    const TCAMProgram& input_program) {
  // Reset all state
  analysis_results_.clear();
  active_passes_.clear();
  tcam_programs_.clear();
  tcam_programs_.push_back(input_program);

  for (auto& pass : transform_passes_) {
    analysis_results_.clear();
    active_passes_.push_back(pass->Name());
    auto new_program = pass->Run(tcam_programs_.back());
    if (new_program.ok()) {
      tcam_programs_.push_back(new_program.value());
    } else {
      // This pass has failed, issue a warning
      ::error("The transformation pass %s has failed: %s",
              std::string(pass->Name()),
              std::string(new_program.status().message()));
      return absl::nullopt;
    }
    active_passes_.pop_back();
  }
  return tcam_programs_.back();
}

}  // namespace backends::tc
