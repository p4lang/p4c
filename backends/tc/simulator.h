#ifndef BACKENDS_TC_SIMULATOR_H_
#define BACKENDS_TC_SIMULATOR_H_

#include <optional>

#include "backends/tc/instruction.h"
#include "backends/tc/tcam_program.h"
#include "backends/tc/util.h"
#include "absl/container/flat_hash_map.h"

namespace backends::tc {
// Simulator for TCAM-based parsers described by `TCAMProgram`. This simulator
// provides a straightforward reference implementation that is not particularly
// efficient. The goal of this simulator is to test the compiler, and to have a
// reference to test more efficient future implementations against.
class ParserSimulator final {
 public:
  explicit ParserSimulator(TCAMProgram tcam_program)
      : tcam_program_(std::move(tcam_program)) {}

  // Parses a single packet from the packet buffer. Returns the final cursor
  // position on success. The extracted fields can be accessed through
  // `extracted_header_fields()` after success.
  absl::optional<size_t> Parse(const std::vector<bool>& packet_buffer);

  const absl::flat_hash_map<HeaderField, Value>& extracted_header_fields()
      const {
    return extracted_header_fields_;
  }

 private:
  // Configuration (in the sense of automata) of the TCAM machine except the
  // extracted headers.
  struct ParserConfiguration {
    State state;
    Value key;
    size_t cursor;
  };

  // Executes given instruction, updates only the new configuration. Returns
  // whether this instruction is successfully executed.
  bool Execute(const Instruction& instruction);

  // Reads given range of bits (relative to the current cursor) from the packet
  // buffer. Returns `absl::nullopt` if the buffer is too short.
  absl::optional<Value> Read(util::Range range);

  TCAMProgram tcam_program_;
  // Fields that are extracted during parsing
  absl::flat_hash_map<HeaderField, Value> extracted_header_fields_;
  // Pointer to the packet buffer, this is only valid while `Parse` is running
  const std::vector<bool>* packet_buffer_;
  // Current and next configuration of the TCAM machine, keeping them separate
  // is necessary to make sure all instructions in an entry see the same
  // configuration.
  ParserConfiguration current_config_;
  ParserConfiguration next_config_;
};
}  // namespace backends::tc

#endif  // BACKENDS_TC_SIMULATOR_H_
