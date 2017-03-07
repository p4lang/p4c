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

#include "common.h"

#include <bm/Standard.h>

#include <algorithm>  // for std::copy

namespace pibmv2 {

Buffer::Buffer() {
  data.reserve(2048);
}

char *
Buffer::extend(size_t s) {
  const auto size = data.size();
  data.resize(size + s);
  return &data.at(size);
}

char *
Buffer::copy() const {
  char *res = new char[data.size()];
  std::copy(data.begin(), data.end(), res);
  return res;
}

size_t
Buffer::size() const {
  return data.size();
}

}  // namespace pibmv2
