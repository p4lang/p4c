#include "backends/tc/tcam_program.h"

#include <algorithm>
#include <numeric>

#include "absl/strings/match.h"
#include "absl/strings/string_view.h"
#include "backends/tc/util.h"

namespace backends::tc {

std::ostream& operator<<(std::ostream& out, const TCAMEntry& entry) {
  out << "TCAMEntry { value = " << util::BitStringToString(entry.value)
      << " mask = " << util::BitStringToString(entry.mask)
      << " instructions = [ ";
  for (const auto& instruction : entry.instructions) {
    out << instruction->ToString() << "; ";
  }
  out << "] }";
  return out;
}

size_t TCAMProgram::HeaderSize(absl::string_view header_name) const {
  if (auto header_fields = HeaderFields(header_name)) {
    size_t header_size = 0;
    for (const auto& field : *header_fields) {
      header_size += field.second;
    }
    return header_size;
  } else {
    // The header's type is unknown, assume it is zero
    return 0;
  }
}

const std::vector<std::pair<FieldPath, size_t>>* TCAMProgram::HeaderFields(
    absl::string_view header_name) const {
  auto header_type = header_instantiations_.find(header_name);
  if (header_type == header_instantiations_.end()) {
    // The header's type is unknown
    return nullptr;
  }
  return &header_type_definitions_.at(header_type->second);
}

absl::optional<std::pair<size_t, size_t>> TCAMProgram::FieldOffsetAndSize(
    const FieldPath& field_path) const {
  const auto header_end = field_path.find('.');
  const auto header = field_path.substr(0, header_end);
  const auto& header_type = header_instantiations_.at(header);
  if (header_end == std::string::npos) {
    // The field_path refers to the whole header, the offset is 0, the size is
    // the total size of the header
    const auto& header_typedef = header_type_definitions_.at(header_type);
    // When switching to C++17, the calculation of `header_size` can be
    // implemented with `std::transform_reduce`.
    size_t header_size = 0;
    for (const auto& field : header_typedef) {
      header_size += field.second;
    }
    return std::pair<size_t, size_t>{0, header_size};
  }
  size_t offset = 0;
  const auto field = field_path.substr(header_end + 1);
  for (const auto& current_field_and_size :
       header_type_definitions_.at(header_type)) {
    const auto& current_field = current_field_and_size.first;
    const auto& size = current_field_and_size.second;
    if (field == current_field) {
      return std::pair<size_t, size_t>{offset, size};
    }
    offset += size;
  }

  // The field is not defined in the header's type
  return absl::nullopt;
}

void TCAMProgram::DumpTcCommands(std::ostream& out) const {
  out << "# header type declarations\n";
  for (const auto& type_def : this->header_type_definitions_) {
    auto& header = type_def.first;
    auto& fields = type_def.second;
    out << "tc declare-header " << header;
    for (const auto& field_and_size : fields) {
      out << " " << field_and_size.first << ":" << field_and_size.second;
    }
    out << "\n";
  }

  out << "\n# header instantiations\n";
  for (const auto& header_and_type : this->header_instantiations_) {
    auto& header = header_and_type.first;
    auto& type = header_and_type.second;
    out << "tc add-header-instance " << header << " type " << type << "\n";
  }

  out << "\n# productions\n";
  for (const auto& i : this->tcam_table_) {
    auto& state = i.first;
    auto& entries = i.second;
    out << "# productions for state '" << state << "'\n";
    for (const auto& entry : entries) {
      out << "tc add-production " << state << " ";
      util::PrettyPrintBitString(entry.value, out);
      out << " ";
      util::PrettyPrintBitString(entry.mask, out);
      for (const auto& instruction : entry.instructions) {
        out << " " << instruction->ToString();
      }
      out << "\n";
    }
  }
}

TCAMEntry* TCAMProgram::FindTCAMEntry(const State& state, const Value& value,
                                      const Mask& mask) {
  auto tcam_entries = this->tcam_table_.find(state);
  if (tcam_entries == this->tcam_table_.end()) {
    // There are no entries for this state
    return nullptr;
  }
  auto& tcam_vec = tcam_entries->second;
  auto tcam_entry = std::find_if(
      tcam_vec.begin(), tcam_vec.end(), [&](const TCAMEntry& entry) {
        return entry.value == value && entry.mask == mask;
      });
  if (tcam_entry == tcam_vec.end()) {
    return nullptr;
  }
  return &*tcam_entry;
}
const TCAMEntry* TCAMProgram::FindTCAMEntry(const State& state,
                                            const Value& value,
                                            const Mask& mask) const {
  auto tcam_entries = this->tcam_table_.find(state);
  if (tcam_entries == this->tcam_table_.end()) {
    // There are no entries for this state
    return nullptr;
  }
  auto& tcam_vec = tcam_entries->second;
  auto tcam_entry = std::find_if(
      tcam_vec.begin(), tcam_vec.end(), [&](const TCAMEntry& entry) {
        return entry.value == value && entry.mask == mask;
      });
  if (tcam_entry == tcam_vec.end()) {
    return nullptr;
  }
  return &*tcam_entry;
}
// Get the specified TCAM entry, insert an empty one if needed
TCAMEntry& TCAMProgram::FindOrInsertTCAMEntry(const State& state,
                                              const Value& value,
                                              const Mask& mask) {
  if (auto tcam_entry = FindTCAMEntry(state, value, mask)) {
    return *tcam_entry;
  }
  // Find the TCAM entries for the state; if there are none, then create an
  // empty vector of entries to associate with this state and return that
  // vector.
  auto& tcam_vec = tcam_table_.try_emplace(std::string{state}).first->second;
  auto tcam_entry =
      std::find_if(tcam_vec.begin(), tcam_vec.end(), [&](TCAMEntry& entry) {
        return entry.value == value && entry.mask == mask;
      });
  if (tcam_entry == tcam_vec.end()) {
    tcam_vec.push_back(TCAMEntry{value, mask, {}});
    return *tcam_vec.rbegin();
  }
  return *tcam_entry;
}
// Insert an empty TCAM entry
TCAMEntry& TCAMProgram::InsertTCAMEntry(const State& state, Value value,
                                        Mask mask) {
  auto& tcam_vec = tcam_table_.try_emplace(std::string{state}).first->second;
  tcam_vec.push_back(TCAMEntry{value, mask, {}});
  return *tcam_vec.rbegin();
}

// Find the entry that matches given state and key according to longest prefix
// match.
const TCAMEntry* TCAMProgram::FindMatchingEntry(const State& state,
                                                const Value& key) const {
  auto entries = tcam_table_.find(state);
  if (entries == tcam_table_.end()) {
    return nullptr;
  }

  const TCAMEntry* lpm_entry = nullptr;
  absl::optional<size_t> match_length;
  for (const auto& entry : entries->second) {
    if (key.size() < entry.mask.size()) {
      // The key is too short for this mask
      continue;
    }

    size_t prefix_length = 0;
    bool fail = false;
    for (size_t i = 0; i < entry.mask.size(); ++i) {
      if (entry.mask[i]) {
        if (key[i] != entry.value[i]) {
          fail = true;
          break;
        }
        prefix_length = i + 1;
      }
    }
    if (!fail && (!match_length || prefix_length > *match_length)) {
      // this entry is the current longest-prefix match
      lpm_entry = &entry;
      match_length = prefix_length;
    }
  }

  return lpm_entry;
}

}  // namespace backends::tc
