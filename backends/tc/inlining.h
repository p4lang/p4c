#ifndef BACKENDS_TC_INLINING_H_
#define BACKENDS_TC_INLINING_H_

#include "backends/tc/pass_manager.h"

// The inlining pass that merges the instructions from the next state if there
// is only one entry for the next state.

namespace backends::tc {

class Inlining final : public TransformPass {
 public:
  explicit Inlining(PassManager& pass_manager) : TransformPass(pass_manager) {}

  absl::string_view Name() const override { return "Inlining"; }

  absl::StatusOr<TCAMProgram> Run(const TCAMProgram& tcam_program) override;
};

}  // namespace backends::tc

#endif  // BACKENDS_TC_INLINING_H_
