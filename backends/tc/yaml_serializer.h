#ifndef BACKENDS_TC_YAML_SERIALIZER_H_
#define BACKENDS_TC_YAML_SERIALIZER_H_

#include "backends/tc/tcam_program.h"

namespace backends::tc {

// Serializes the TCAM program to a series of tc commands, organized in YAML.
absl::StatusOr<std::string> SerializeToYaml(const TCAMProgram& program);

}  // namespace backends::tc

#endif  // BACKENDS_TC_YAML_SERIALIZER_H_
