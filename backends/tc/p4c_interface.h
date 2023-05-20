#ifndef BACKENDS_TC_P4C_INTERFACE_H_
#define BACKENDS_TC_P4C_INTERFACE_H_

#include "control-plane/p4RuntimeSerializer.h"
#include "frontends/common/applyOptionsPragmas.h"
#include "frontends/common/options.h"
#include "frontends/common/parseInput.h"
#include "frontends/p4/evaluator/evaluator.h"
#include "frontends/p4/frontend.h"
#include "frontends/p4/toP4/toP4.h"
#include "ir/ir.h"
#include "ir/json_loader.h"
#include "lib/crash.h"
#include "lib/error.h"
#include "lib/exceptions.h"
#include "lib/gc.h"
#include "lib/log.h"
#include "lib/nullstream.h"

// The constructs used for interacting with P4C's frontend and midend.

namespace backends::tc {

// P4C mid-end that operates over P4 IR. This mid-end loads and runs the
// evaluator pass to simplify the IR for us.
class MidEnd : public ::PassManager {
 public:
  P4::ReferenceMap ref_map_;
  P4::TypeMap type_map_;
  IR::ToplevelBlock *toplevel_ = nullptr;

  explicit MidEnd(CompilerOptions &options) {
    bool isv1 = options.langVersion == CompilerOptions::FrontendVersion::P4_14;
    ref_map_.setIsV1(isv1);
    auto evaluator = new P4::EvaluatorPass(&ref_map_, &type_map_);
    setName("MidEnd");

    addPasses({
        evaluator,
        [this, evaluator]() { toplevel_ = evaluator->getToplevelBlock(); },
    });
  }
  IR::ToplevelBlock *process(const IR::P4Program *&program) {
    program = program->apply(*this);
    return toplevel_;
  }
};

// Compiler options for the tc backend
class P4TcOptions : public CompilerOptions {
 public:
  bool load_ir_from_json_ = false;
  // name of the file to write the compiler out to
  absl::optional<std::string> output_file_;
  // whether to dump tc commands for the TCAM program
  bool dump_tc_commands_ = false;

  P4TcOptions() {
    registerOption(
        "--fromJSON", "file",
        [this](const char *file_name) {
          this->load_ir_from_json_ = true;
          this->file = file_name;
          return true;
        },
        "read P4 IR from given JSON file, rather than P4 source code.");
    registerOption(
        "--dump-tc-commands", nullptr,
        [this](const char *_) {
          this->dump_tc_commands_ = true;
          return true;
        },
        "Dump tc commands to standard output.");
    registerOption(
        "-o", "outfile",
        [this](const char *file_name) {
          this->output_file_ = file_name;
          return true;
        },
        "Write the output to outfile rather than the standard output.");
  }

  virtual ~P4TcOptions() {}
};

using P4TcContext = P4CContextWithOptions<P4TcOptions>;

}  // namespace backends::tc

#endif  // BACKENDS_TC_P4C_INTERFACE_H_
