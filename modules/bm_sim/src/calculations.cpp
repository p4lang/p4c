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

#include "bm_sim/calculations.h"

#include "xxhash.h"

namespace hash {

template <typename T,
	  typename std::enable_if<std::is_unsigned<T>::value, int>::type = 0>
T xxh64(const char *buf, size_t len) {
  return static_cast<T>(XXH64(buf, len, 0));
}

template unsigned int xxh64<unsigned int>(const char *, size_t);

}


