#ifndef BACKENDS_TC_INSTRUCTION_H_
#define BACKENDS_TC_INSTRUCTION_H_

// Instructions used in the parser productions. See `Instruction` base class for
// more details.

#include <string>
#include <utility>
#include <vector>

#include "absl/hash/hash.h"
#include "backends/tc/util.h"
#include "lib/exceptions.h"

namespace backends::tc {

// Values and masks used in TCAM entries, represented as bit vectors.
using Value = std::vector<bool>;
using Mask = std::vector<bool>;
using State = std::string;
using HeaderField = std::string;

// The abstract base class for all instructions. The instructions are
// implemented as immutable objects.
class Instruction {
 public:
  virtual std::string ToString() const = 0;
  virtual bool operator==(const Instruction &that) const = 0;
  bool operator!=(const Instruction &that) const { return !(*this == that); }
  virtual ~Instruction() = default;

  // Compute the hash value of instructions so that instruction objects can be
  // placed in Abseil's hash tables, or used in hashes of other container
  // objects such as `TCAMEntry`.
  template <typename H>
  friend H AbslHashValue(H h, const Instruction &instruction);
};

// Move the cursor by the given number of bits
class Move final : public Instruction {
 public:
  explicit Move(uint32_t offset) : offset_(offset) {}
  inline uint32_t offset() const { return this->offset_; }
  std::string ToString() const final;
  bool operator==(const Instruction &that) const override;

  static constexpr absl::string_view kOpcode = "move";

 private:
  uint32_t offset_;
};

// Set the next state
class NextState final : public Instruction {
 public:
  explicit NextState(State state) : state_(state) {}
  NextState(NextState &&) = default;
  inline const State &state() const { return this->state_; }
  std::string ToString() const final;
  bool operator==(const Instruction &that) const override;

  static constexpr absl::string_view kOpcode = "set-next-state";

 private:
  const State state_;
};

// Set the look-up key to a range of the packet buffer
class SetKey final : public Instruction {
 public:
  explicit SetKey(util::Range range) : range_(std::move(range)) {}
  std::string ToString() const final;
  inline const util::Range &range() const { return this->range_; }
  bool operator==(const Instruction &that) const override;

  static constexpr absl::string_view kOpcode = "set-key";

 private:
  util::Range range_;
};

// Store given range in the packet buffer as given header field
class StoreHeaderField : public Instruction {
 public:
  StoreHeaderField(util::Range range, HeaderField header_field)
      : range_(std::move(range)), header_field_(std::move(header_field)) {}
  inline const std::string &header_field() const { return this->header_field_; }
  inline const util::Range &range() const { return this->range_; }
  std::string ToString() const final;
  bool operator==(const Instruction &that) const override;

  static constexpr absl::string_view kOpcode = "store";

 private:
  util::Range range_;
  const std::string header_field_;
};

// Hasher implementation, it needs to be out-of-line because it uses the fields
// from the derived classes.
template <typename H>
H AbslHashValue(H h, const Instruction &instruction) {
  // Match instruction type, then extract the values to hash over
  if (const auto move = dynamic_cast<const Move *>(&instruction)) {
    return H::combine(std::move(h), move->offset());
  } else if (const auto set_key = dynamic_cast<const SetKey *>(&instruction)) {
    return H::combine(std::move(h), set_key->range().from(),
                      set_key->range().to());
  } else if (const auto next_state =
                 dynamic_cast<const NextState *>(&instruction)) {
    return H::combine(std::move(h), next_state->state());
  } else {
    const auto store = dynamic_cast<const StoreHeaderField *>(&instruction);
    BUG_CHECK(store, "The type of the instruction '%s' is not accounted for.",
              instruction.ToString());
    return H::combine(std::move(h), store->range().from(), store->range().to(),
                      store->header_field());
  }
}

}  // namespace backends::tc

#endif  // BACKENDS_TC_INSTRUCTION_H_
