#include "backends/tc/instruction.h"

#include <sstream>
#include <string>
#include <utility>

namespace backends::tc {

// Opcode definitions, these should match the values in `instruction.h`. We need
// these in C++11 because of storage requirements. These can be removed once we
// switch to C++17.
constexpr absl::string_view Move::kOpcode;
constexpr absl::string_view NextState::kOpcode;
constexpr absl::string_view SetKey::kOpcode;
constexpr absl::string_view StoreHeaderField::kOpcode;

std::string Move::ToString() const {
  std::ostringstream out;
  out << kOpcode << " " << this->offset();
  return std::move(out).str();
}

bool Move::operator==(const Instruction &that) const {
  if (const auto that_move = dynamic_cast<const Move *>(&that)) {
    return offset() == that_move->offset();
  }
  return false;
}

std::string NextState::ToString() const {
  std::ostringstream out;
  out << kOpcode << " " << this->state();
  return std::move(out).str();
}

bool NextState::operator==(const Instruction &that) const {
  if (const auto that_next_state = dynamic_cast<const NextState *>(&that)) {
    return state() == that_next_state->state();
  }
  return false;
}

std::string SetKey::ToString() const {
  std::ostringstream out;
  out << kOpcode << " " << this->range();
  return std::move(out).str();
}

bool SetKey::operator==(const Instruction &that) const {
  if (const auto that_set_key = dynamic_cast<const SetKey *>(&that)) {
    return range() == that_set_key->range();
  }
  return false;
}

std::string StoreHeaderField::ToString() const {
  std::ostringstream out;
  out << kOpcode << " " << this->range() << " " << this->header_field();
  return std::move(out).str();
}

bool StoreHeaderField::operator==(const Instruction &that) const {
  if (const auto that_store = dynamic_cast<const StoreHeaderField *>(&that)) {
    return range() == that_store->range() &&
           header_field() == that_store->header_field();
  }
  return false;
}

}  // namespace backends::tc
