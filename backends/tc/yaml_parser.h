#ifndef BACKENDS_TC_YAML_PARSER_H_
#define BACKENDS_TC_YAML_PARSER_H_

#include "absl/status/statusor.h"
#include "absl/strings/string_view.h"
#include "backends/tc/tcam_program.h"

namespace backends::tc {

// Parses given YAML-encoded list of tc commands to create a TCAM abstract
// machine program.
absl::StatusOr<TCAMProgram> ParseYaml(const std::string& yaml_string);

}  // namespace backends::tc

#endif  // BACKENDS_TC_YAML_PARSER_H_
