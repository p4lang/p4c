#include "backends/tc/yaml_parser.h"

#include <memory>
#include <vector>

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_split.h"
#include "absl/strings/string_view.h"
#include "backends/tc/instruction.h"
#include "backends/tc/tcam_program.h"
#include "backends/tc/util.h"
#include "yaml-cpp/yaml.h"

namespace backends::tc {

namespace {}

// Parses given YAML-encoded list of tc commands to create a TCAM abstract
// machine program.
//
// See the README for the specific YAML encoding.
absl::StatusOr<TCAMProgram> ParseYaml(const std::string& yaml_string) {
  TCAMProgram tcam_program;
  try {
    const auto program_node = YAML::Load(yaml_string);
    if (!program_node.IsDefined() || !program_node.IsMap()) {
      return absl::InvalidArgumentError(
          "The YAML string is ill-formatted. Expected a map at the top level.");
    }
    // Parse header type declarations. Header type declarations are of the
    // form
    //
    //   header-name field1:size1 field2:size2 ...
    //
    // For example: ethernet dstAddr:48 srcAddr:48 etherType:16
    auto header_declarations = program_node["declare-header"]
                                   .as<std::vector<std::vector<std::string>>>();
    for (const auto& header_declaration : header_declarations) {
      const auto& type_name = header_declaration.front();
      std::vector<std::pair<FieldPath, size_t>> field_sizes;
      for (auto i = header_declaration.begin() + 1,
                end = header_declaration.end();
           i != end; ++i) {
        std::vector<std::string> field_and_size = absl::StrSplit(*i, ':');
        if (field_and_size.size() != 2) {
          return absl::InvalidArgumentError(
              "The header fields should be encoded as field:size. There are "
              "more than 1 ':' in a field description");
        }
        auto size = util::ParseUInt32(field_and_size.at(1));
        if (!size.ok()) {
          return size.status();
        }
        field_sizes.push_back({field_and_size.at(0), *size});
      }
      tcam_program.header_type_definitions_.emplace(HeaderType(type_name),
                                                    std::move(field_sizes));
    }

    // Parse header instances. They are of the form "header-name header-type"
    auto header_instances = program_node["add-header-instance"]
                                .as<std::vector<std::vector<std::string>>>();
    for (const auto& instance : header_instances) {
      if (instance.size() != 2 ||
          !tcam_program.header_type_definitions_.contains(instance.at(1))) {
        return absl::InvalidArgumentError(
            "Header instance declarations should contain only a header name "
            "and a valid header type name");
      }
      tcam_program.header_instantiations_.emplace(instance.at(0),
                                                  instance.at(1));
    }

    // Parse state transitions. The state transitions are of the form
    //
    //  state value mask instructions...
    //
    // We extract the state, the value, and the mask; then we use a state
    // machine to parse the instructions.
    auto state_transitions = program_node["add-transition"]
                                 .as<std::vector<std::vector<std::string>>>();
    for (const auto& transition : state_transitions) {
      if (transition.size() < 3) {
        return absl::InvalidArgumentError(
            "Each state transition should have a state, a value, and a mask");
      }
      auto i = transition.begin();
      auto end = transition.end();
      State state = *i++;
      auto value = util::SizedStringToBitString(*i++);
      if (!value.ok()) {
        return value.status();
      }
      auto mask = util::SizedStringToBitString(*i++);
      if (!mask.ok()) {
        return mask.status();
      }
      auto& tcam_entry = tcam_program.InsertTCAMEntry(state, *value, *mask);
      // Get the next token if possible
      auto next = [&i, &end]() -> absl::StatusOr<absl::string_view> {
        if (i != end) {
          return *i++;
        }
        return absl::InvalidArgumentError(
            "The instruction list ends prematurely.");
      };
      // Parse instructions
      while (i != end) {
        auto instruction = next();
        if (!instruction.ok()) {
          return instruction.status();
        }
        if (*instruction == Move::kOpcode) {
          auto offset_token = next();
          if (!offset_token.ok()) {
            return offset_token.status();
          }
          auto offset = util::ParseUInt32(*offset_token);
          if (!offset.ok()) {
            return offset.status();
          }
          tcam_entry.instructions.push_back(std::make_shared<Move>(*offset));
        } else if (*instruction == StoreHeaderField::kOpcode) {
          auto range_token = next();
          if (!range_token.ok()) {
            return range_token.status();
          }
          auto range = util::Range::Parse(*range_token);
          if (!range.ok()) {
            return range.status();
          }
          auto header_field = next();
          if (!header_field.ok()) {
            return header_field.status();
          }
          tcam_entry.instructions.push_back(std::make_shared<StoreHeaderField>(
              *range, HeaderField(*header_field)));
        } else if (*instruction == NextState::kOpcode) {
          auto next_state = next();
          if (!next_state.ok()) {
            return next_state.status();
          }
          tcam_entry.instructions.push_back(
              std::make_shared<NextState>(State(*next_state)));
        } else if (*instruction == SetKey::kOpcode) {
          auto range_token = next();
          if (!range_token.ok()) {
            return range_token.status();
          }
          auto range = util::Range::Parse(*range_token);
          if (!range.ok()) {
            return range.status();
          }
          tcam_entry.instructions.push_back(std::make_shared<SetKey>(*range));
        } else {
          std::cerr << "invalid insn: " << *instruction << "\n";
          return absl::InvalidArgumentError(
              "Encountered unknown instruction when parsing a stream of "
              "instructions");
        }
      }
    }
  } catch (YAML::ParserException& e) {
    return absl::InvalidArgumentError(
        "The input to ParseYaml is not a valid YAML string");
  } catch (YAML::InvalidNode& e) {
    return absl::InvalidArgumentError(
        "The input YAML file is not the correct format.");
  }

  return tcam_program;
}

}  // namespace backends::tc
