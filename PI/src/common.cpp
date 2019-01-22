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

#include <algorithm>  // for std::copy

namespace pibmv2 {

Buffer::Buffer(size_t capacity) {
  data_.reserve(capacity);
}

char *
Buffer::extend(size_t s) {
  if (s == 0) return nullptr;
  const auto size = data_.size();
  data_.resize(size + s);
  return &data_[size];
}

char *
Buffer::copy() const {
  char *res = new char[data_.size()];
  std::copy(data_.begin(), data_.end(), res);
  return res;
}

char *
Buffer::data() {
  return data_.data();
}

size_t
Buffer::size() const {
  return data_.size();
}

}  // namespace pibmv2
