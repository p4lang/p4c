#ifndef BACKENDS_TC_TCAM_PROGRAM_H_
#define BACKENDS_TC_TCAM_PROGRAM_H_

#include <memory>
#include <optional>
#include <ostream>
#include <string>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "absl/hash/hash.h"
#include "backends/tc/instruction.h"

namespace backends::tc {

using HeaderType = std::string;
using HeaderName = std::string;
using FieldPath = std::string;

// Names of special states
constexpr absl::string_view kStartState = "start";
constexpr absl::string_view kAcceptState = "accept";
constexpr absl::string_view kRejectState = "reject";

inline bool IsSpecial(absl::string_view state) {
  return state == kStartState || state == kAcceptState || state == kRejectState;
}

// An entry in the TCAM, note that we factor out the state because some
// operations such as checking for conflicts between different entries, or
// checking which entries match a given key requires traversing all values and
// masks for the current state. Factoring out the state allows us to check only
// entries relevant to the current state rather than the whole TCAM table during
// these operations.
struct TCAMEntry final {
  Value value;
  Mask mask;
  // Instructions use virtual inheritance so we have to have a vector of
  // pointers. We use a vector of shared_ptrs to const objects so that this
  // struct is copyable by default and it does not make an allocation per
  // instruction when copying.
  std::vector<std::shared_ptr<const Instruction>> instructions;

  template <typename H>
  friend H AbslHashValue(H h, const TCAMEntry &entry) {
    h = H::combine(std::move(h), entry.value, entry.mask);
    // Hash instructions by value
    for (const auto &instruction : entry.instructions) {
      h = H::combine(std::move(h), *instruction);
    }
    return h;
  }

  bool operator==(const TCAMEntry &that) const {
    if (this->value != that.value || this->mask != that.mask ||
        this->instructions.size() != that.instructions.size()) {
      return false;
    }
    // Compare instructions by value. This comparison still respects the order,
    // so two sequences of instructions are different if their ordering differs,
    // even if their effect is the same.
    //
    // For example:
    // [move 16; set-next-state S] != [set-next-state S; move 16]
    for (size_t i = 0; i < instructions.size(); ++i) {
      if (*this->instructions[i] != *that.instructions[i]) {
        return false;
      }
    }
    return true;
  }

  bool operator!=(const TCAMEntry &that) const { return !(*this == that); }

  // operator<< to allow printing to streams for debugging and logging.
  //
  // This is not used for YAML serialization!
  friend std::ostream &operator<<(std::ostream &, const TCAMEntry &);
};

// TCAM factored by state, we need to check all TCAM entries for a state, so we
// can store them in a vector to iterate over linearly. This table should never
// change while the parser is running.
using TCAMTable = absl::flat_hash_map<State, std::vector<TCAMEntry>>;

// The TCAM program we are using as our intermediate representation.
class TCAMProgram final {
 public:
  TCAMTable tcam_table_;
  absl::flat_hash_map<HeaderName, HeaderType> header_instantiations_;
  absl::flat_hash_map<HeaderType, std::vector<std::pair<FieldPath, size_t>>>
      header_type_definitions_;

  // Dump the tc commands that describe this program, the comment lines start
  // with a "#" potentially after some white space.
  void DumpTcCommands(std::ostream &out) const;

  // Calculate the size of given header type. This currently traverses all field
  // paths. If it becomes a performance bottleneck, we can implement
  // memoization.
  size_t HeaderSize(absl::string_view header_name) const;

  // Get the fields of given header by looking up its type's definition.
  const std::vector<std::pair<FieldPath, size_t>> *HeaderFields(
      absl::string_view header_name) const;

  // Get the offset and size of a field relative to the header its defined in.
  absl::optional<std::pair<size_t, size_t>> FieldOffsetAndSize(
      const FieldPath &field) const;

  TCAMEntry *FindTCAMEntry(const State &, const Value &, const Mask &);
  const TCAMEntry *FindTCAMEntry(const State &, const Value &,
                                 const Mask &) const;
  // Get the specified TCAM entry, insert an empty one if needed
  TCAMEntry &FindOrInsertTCAMEntry(const State &, const Value &, const Mask &);
  // Insert an empty TCAM entry, removing the old one if there is any
  TCAMEntry &InsertTCAMEntry(const State &, Value, Mask);

  // Find the entry that matches given state and key according to longest prefix
  // match.
  const TCAMEntry *FindMatchingEntry(const State &state,
                                     const Value &key) const;
};

}  // namespace backends::tc

#endif  // BACKENDS_TC_TCAM_PROGRAM_H_
