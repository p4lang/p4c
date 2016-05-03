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

#include <vector>

namespace bm {

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
    registers_new.emplace_back(bitwidth);
  auto lock = UniqueLock();
  registers.swap(registers_new);
}


void
RegisterSync::add_register_array(RegisterArray *register_array) {
  if (register_arrays.insert(register_array).second)
    mutexes.push_back(&register_array->m_mutex);
}

}  // namespace bm
