#include "backends/tc/simulator.h"

#include <optional>

#include "backends/tc/instruction.h"
#include "backends/tc/tcam_program.h"
#include "backends/tc/util.h"
#include "absl/container/flat_hash_map.h"

namespace backends::tc {
absl::optional<Value> ParserSimulator::Read(util::Range range) {
  range = range.OffsetBy(current_config_.cursor);
  if (range.to() > packet_buffer_->size()) {
    return absl::nullopt;
  }
  return Value(packet_buffer_->begin() + range.from(),
               packet_buffer_->begin() + range.to());
}

bool ParserSimulator::Execute(const Instruction &instruction) {
  if (auto move = dynamic_cast<const Move *>(&instruction)) {
    next_config_.cursor += move->offset();
    if (next_config_.cursor > packet_buffer_->size()) {
      // moved beyond the packet buffer
      return false;
    }
  } else if (auto set_key = dynamic_cast<const SetKey *>(&instruction)) {
    auto new_key = Read(set_key->range());
    if (!new_key) {
      return false;
    }
    next_config_.key = *std::move(new_key);
  } else if (auto next_state = dynamic_cast<const NextState *>(&instruction)) {
    next_config_.state = next_state->state();
  } else {
    auto store = dynamic_cast<const StoreHeaderField *>(&instruction);
    BUG_CHECK(store, "How to simulate the instruction %s is not implemented",
              instruction.ToString());
    auto new_value = Read(store->range());
    if (!new_value) {
      return false;
    }
    extracted_header_fields_.insert_or_assign(store->header_field(),
                                              *new_value);
  }
  return true;
}

absl::optional<size_t> ParserSimulator::Parse(
    const std::vector<bool> &packet_buffer) {
  extracted_header_fields_.clear();
  packet_buffer_ = &packet_buffer;
  current_config_.key.clear();
  current_config_.state = State(kStartState);
  current_config_.cursor = 0;

  while (current_config_.state != kAcceptState &&
         current_config_.state != kRejectState) {
    auto entry = tcam_program_.FindMatchingEntry(current_config_.state,
                                                 current_config_.key);
    if (!entry) {
      // There is no matching entry, go to reject state
      current_config_.state = State(kRejectState);
      current_config_.key.clear();
      break;
    }
    next_config_.cursor = current_config_.cursor;
    // clear the key
    next_config_.key.clear();
    // execute the instructions
    for (const auto &instruction : entry->instructions) {
      if (!Execute(*instruction)) {
        // failed to run an instruction because the packet buffer is too short
        return absl::nullopt;
      }
    }
    current_config_ = std::move(next_config_);
  }

  if (current_config_.state == kRejectState) {
    return absl::nullopt;
  }
  return current_config_.cursor;
}
}  // namespace backends::tc
