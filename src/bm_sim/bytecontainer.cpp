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
#include <sstream>

#include "utils.h"

namespace bm {

std::string
ByteContainer::to_hex(size_t start, size_t s, bool upper_case) const {
  assert(start + s <= size());

  std::ostringstream ret;
  utils::dump_hexstring(ret, &bytes[start], &bytes[start + s], upper_case);
  return ret.str();
}

}  // namespace bm
