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

#include <bm/bm_sim/bytecontainer.h>

#include <string>
#include <iostream>
#include <sstream>

namespace bm {

std::string
ByteContainer::to_hex(size_t start, size_t s, bool upper_case) const {
  assert(start + s <= size());

  std::ostringstream ret;

  for (std::string::size_type i = start; i < start + s; i++) {
    ret << std::setw(2) << std::setfill('0') << std::hex
        << (upper_case ? std::uppercase : std::nouppercase)
        // the int cast was not sufficient 0xab -> 0xffffffab
        // << static_cast<int>(bytes[i]);
        << static_cast<int>(static_cast<unsigned char>(bytes[i]));
  }

  return ret.str();
}

}  // namespace bm
