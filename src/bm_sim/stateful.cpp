/* Copyright 2013-present Barefoot Networks, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * Antonin Bas (antonin@barefootnetworks.com)
 *
 */

#include <bm/bm_sim/stateful.h>

#include <iterator>  // std::distance
#include <string>
#include <vector>

namespace bm {

Register::Register(int nbits, const RegisterArray *register_array)
    : nbits(nbits), register_array(register_array) {
  mask <<= nbits; mask -= 1;
}

void
Register::export_bytes() {
  value &= mask;
  register_array->notify(*this);
}

RegisterArray::RegisterArray(const std::string &name, p4object_id_t id,
                             size_t size, int bitwidth)
    : NamedP4Object(name, id), bitwidth(bitwidth) {
  registers.reserve(size);
  for (size_t i = 0; i < size; i++)
    registers.emplace_back(bitwidth, this);
}

void
RegisterArray::reset_state() {
  // we build a new vector of registers, then swap, to avoid holding the lock
  // for too long
  std::vector<Register> registers_new;
  size_t s = size();
  // TODO(antonin): is this actually better than
  // std::vector<Register> registers_new(size, Register(bitwidth)); ?
  registers_new.reserve(s);
  for (size_t i = 0; i < s; i++)
    registers_new.emplace_back(bitwidth, this);
  auto lock = UniqueLock();
  registers.swap(registers_new);
}

void
RegisterArray::register_notifier(Notifier notifier) {
  notifiers.push_back(std::move(notifier));
}

void
RegisterArray::notify(const Register &reg) const {
  for (const auto &notifier : notifiers)
    notifier(std::distance(&registers[0], &reg));
}

void
RegisterSync::add_register_array(RegisterArray *register_array) {
  if (register_arrays.insert(register_array).second)
    mutexes.push_back(&register_array->m_mutex);
}

}  // namespace bm
