#include "backends/p4tools/common/compiler/compiler_result.h"

namespace P4Tools {

const IR::P4Program &CompilerResult::getProgram() const { return program; }

CompilerResult::CompilerResult(const IR::P4Program &program) : program(program) {}

}  // namespace P4Tools
