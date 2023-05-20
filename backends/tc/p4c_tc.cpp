// The main entry point of the compiler.
//
// This driver defines the compiler options, the P4C mid-end passes to run, and
// the pipeline to pass the P4 program through:
//
// - P4C Front-end
// - P4C Mid-end
// - Translation to TCAM abstract machine
// - TCAM abstract machine mid-end
// - Backend to output metadata and tc commands

#include <fstream>
#include <iostream>
#include <memory>
#include <string>

#include "absl/memory/memory.h"
#include "backends/tc/dead_state_elimination.h"
#include "backends/tc/inlining.h"
#include "backends/tc/ir_builder.h"
#include "backends/tc/p4c_interface.h"
#include "backends/tc/pass_manager.h"
#include "backends/tc/yaml_parser.h"
#include "backends/tc/yaml_serializer.h"

using backends::tc::MidEnd;
using backends::tc::P4TcContext;

static const char *kCompilerVersion = "0.0.0.0";

int main(int argc, char *argv[]) {
  // Set up P4C's infrastructure
  setup_gc_logging();
  setup_signals();

  AutoCompileContext compileContext(new P4TcContext);
  auto &options = P4TcContext::get().options();

  // Support P4-16 only
  options.langVersion = CompilerOptions::FrontendVersion::P4_16;
  options.compilerVersion = kCompilerVersion;

  // Parse arguments and set the input file
  if (options.process(argc, argv) != nullptr) {
    if (options.load_ir_from_json_ == false) options.setInputFile();
  }
  if (::errorCount() > 0) {
    return 1;
  }

  auto debug_hook = options.getDebugHook();

  // Compute or load the IR
  const IR::P4Program *program = nullptr;

  if (options.load_ir_from_json_) {
    std::ifstream json(options.file);
    if (json) {
      JSONLoader loader(json);
      const IR::Node *node = nullptr;
      loader >> node;
      if (!(program = node->to<IR::P4Program>()))
        error(ErrorType::ERR_INVALID, "%s is not a P4Program in json format",
              options.file);
    } else {
      error(ErrorType::ERR_IO, "Can't open %s", options.file);
    }
  } else {
    program = P4::parseP4File(options);

    if (program != nullptr && ::errorCount() == 0) {
      P4::P4COptionPragmaParser optionsPragmaParser;
      program->apply(P4::ApplyOptionsPragmas(optionsPragmaParser));
      try {
        P4::FrontEnd fe;
        fe.addDebugHook(debug_hook);
        program = fe.run(options, program);
      } catch (const std::exception &bug) {
        std::cerr << bug.what() << std::endl;
        return 1;
      }
    }
  }

  // Run the P4C mid-end
  MidEnd mid_end(options);
  mid_end.addDebugHook(debug_hook);
  try {
    mid_end.process(program);
    if (options.dumpJsonFile) {
      JSONGenerator(*openFile(options.dumpJsonFile, true))
          << program << std::endl;
    }
  } catch (const std::exception &error) {
    std::cerr << error.what() << std::endl;
    return 1;
  }

  if (::errorCount() > 0) {
    return 1;
  }

  backends::tc::IRBuilder ir_builder(mid_end.ref_map_, mid_end.type_map_);

  try {
    program->apply(ir_builder);
    if (ir_builder.has_errors()) {
      return 1;
    }
  } catch (backends::tc::IRBuilderError &err) {
    ::error(
        "Could not build the initial TCAM program because of errors in the P4 "
        "program.");
    return 1;
  }

  const auto &tcam_program = ir_builder.tcam_program();

  // Run the passes in the backend's mid-end
  backends::tc::PassManager pass_manager;
  pass_manager.AddPass(absl::make_unique<backends::tc::Inlining>(pass_manager));
  pass_manager.AddPass(
      absl::make_unique<backends::tc::DeadStateElimination>(pass_manager));
  if (const auto final_program = pass_manager.RunPasses(tcam_program)) {
    if (options.dump_tc_commands_) {
      // Dump the tc commands in a form that can be pasted into a shell script.
      final_program->DumpTcCommands(std::cout);
    }
    if (options.output_file_) {
      std::ofstream out(*options.output_file_);
      if (!out) {
        ::error("Could not open the output file %s", *options.output_file_);
      }
      out << SerializeToYaml(*final_program).value() << std::endl;
      if (!out) {
        ::error("Failed to write the output to the file %s",
                *options.output_file_);
      }
    } else {
      std::cout << SerializeToYaml(*final_program).value() << std::endl;
    }
  } else {
    ::error(
        "Some of the transformation passes failed, not writing out the final "
        "program.");
  }

  return ::errorCount() > 0;
}
