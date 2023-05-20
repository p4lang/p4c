#include "backends/tc/yaml_serializer.h"

#include <sstream>

#include "absl/status/status.h"
#include "absl/strings/str_split.h"
#include "absl/strings/string_view.h"
#include "yaml-cpp/emitter.h"
#include "yaml-cpp/emitterstyle.h"
#include "yaml-cpp/yaml.h"

namespace backends::tc {

absl::StatusOr<std::string> SerializeToYaml(const TCAMProgram& program) {
  // The root node of the YAML documents
  YAML::Node tc_commands;
  // Save the header type declarations, sort them to have deterministic output
  std::vector<absl::string_view> header_types;
  for (const auto& type_def : program.header_type_definitions_) {
    header_types.push_back(type_def.first);
  }
  std::sort(header_types.begin(), header_types.end());
  for (const auto& header_type : header_types) {
    YAML::Node header_declaration;
    header_declaration.push_back(HeaderType(header_type));
    for (const auto& field_info :
         program.header_type_definitions_.at(header_type)) {
      std::ostringstream out;
      out << field_info.first << ":" << field_info.second;
      header_declaration.push_back(out.str());
    }
    tc_commands["declare-header"].push_back(header_declaration);
  }
  // Header instances, sort the headers first to have deterministic output
  std::vector<absl::string_view> headers;
  for (const auto& header_instance : program.header_instantiations_) {
    headers.push_back(header_instance.first);
  }
  std::sort(headers.begin(), headers.end());
  for (const auto& header : headers) {
    tc_commands["add-header-instance"].push_back(std::vector<std::string>{
        HeaderName(header), program.header_instantiations_.at(header)});
  }
  // State transitions, sort the states first to have deterministic output
  std::vector<absl::string_view> states;
  for (const auto& state_to_entries : program.tcam_table_) {
    states.push_back(state_to_entries.first);
  }
  std::sort(states.begin(), states.end());

  for (const auto& state : states) {
    for (const auto& entry : program.tcam_table_.at(state)) {
      YAML::Node transition;
      transition.SetStyle(YAML::EmitterStyle::Flow);
      // output state, value, mask
      transition.push_back(State(state));
      std::ostringstream tmp;
      util::PrettyPrintBitString(entry.value, tmp);
      transition.push_back(tmp.str());
      tmp = std::ostringstream();  // reset the output stream
      util::PrettyPrintBitString(entry.mask, tmp);
      transition.push_back(tmp.str());
      // flatten and output instructions
      for (const auto& instruction : entry.instructions) {
        std::vector<std::string> instruction_tokens =
            absl::StrSplit(instruction->ToString(), ' ');
        for (auto&& token : instruction_tokens) {
          transition.push_back(token);
        }
      }
      tc_commands["add-transition"].push_back(transition);
    }
  }

  YAML::Emitter out;
  out << tc_commands;
  if (!out.good()) {
    return absl::InternalError("The YAML serializer has failed.");
  }
  return std::string(out.c_str());
}

}  // namespace backends::tc
