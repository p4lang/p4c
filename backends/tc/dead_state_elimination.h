#ifndef BACKENDS_TC_DEAD_STATE_ELIMINATION_H_
#define BACKENDS_TC_DEAD_STATE_ELIMINATION_H_

#include "backends/tc/pass_manager.h"

// A pass that removes unused states

namespace backends::tc {

class DeadStateElimination final : public TransformPass {
 public:
  explicit DeadStateElimination(PassManager& pass_manager)
      : TransformPass(pass_manager) {}
  absl::string_view Name() const override { return "DeadStateElimination"; }

  absl::StatusOr<TCAMProgram> Run(const TCAMProgram& tcam_program) override;
};

}  // namespace backends::tc

#endif  // BACKENDS_TC_DEAD_STATE_ELIMINATION_H_
