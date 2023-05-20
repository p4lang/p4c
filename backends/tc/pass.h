#ifndef BACKENDS_TC_PASS_H_
#define BACKENDS_TC_PASS_H_

#include <optional>

#include "absl/status/statusor.h"
#include "absl/strings/string_view.h"
#include "backends/tc/tcam_program.h"

// Analysis and transformation passes over TCAM programs.
//
// All analysis passes should inherit from `AnalysisPass<ActualPass, Result>`
// where `ActualPass` is the name of the pass and `Result` is the analysis
// result the pass computes, and all transformation passes should inherit from
// `TransformPass`.

namespace backends::tc {

// Forward declarations
class PassManager;

// A compiler pass, this class carries the common functionality of analysis and
// transformation passes.
class Pass {
 public:
  virtual ~Pass() {}

  // The name of this pass so it can be registered with the pass manager, and
  // its name can be used for logging.
  virtual absl::string_view Name() const = 0;

 protected:
  explicit Pass(PassManager& pass_manager) : pass_manager_(pass_manager) {}

  // Compiler passes hold a reference to the pass manager so that they can
  // request analysis results from the pass manager.
  PassManager& pass_manager_;
};

// An analysis pass with a name and a cacheable result.
//
// Analysis passes should have:
//   - a static string member `kName`, and
//   - a static type named `Result`. `Result` needs to be copy-constructible
//   because it is stored inside an instance of `std::any` when caching results.
//
// This is required by the `GetAnalysisResult` method of `PassManager`. Having
// the name as a static member allows querying analysis results without creating
// an instance. Right now, this is enforced using curiously recurring template
// pattern (a.k.a. F-bounded quantification). In the future, we can enforce this
// more elegantly when we have concepts.
//
// The template parameter `P` is the type of the concrete analysis pass.
template <class P, class Result = typename P::Result>
class AnalysisPass : public Pass {
 public:
  explicit AnalysisPass(PassManager& pass_manager) : Pass(pass_manager) {}

  absl::string_view Name() const final { return P::kName; }

  // Run this analysis pass on the given program.
  virtual absl::StatusOr<Result> Run(const TCAMProgram& tcam_program) = 0;

  ~AnalysisPass() override {}
};

// A transformation pass that computes a new TCAM program out of a given TCAM
// program.
class TransformPass : public Pass {
 public:
  explicit TransformPass(PassManager& pass_manager) : Pass(pass_manager) {}
  // Run this pass on the given program and compute a new program.
  virtual absl::StatusOr<TCAMProgram> Run(const TCAMProgram& tcam_program) = 0;

  ~TransformPass() override {}
};

}  // namespace backends::tc

#endif  // BACKENDS_TC_PASS_H_
