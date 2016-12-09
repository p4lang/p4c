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

#ifndef BM_SIM_UTILS_H_
#define BM_SIM_UTILS_H_

#include <iterator>
#include <iomanip>
#include <ostream>
#include <string>
#include <type_traits>

namespace bm {

namespace utils {

// Saves the internal state of a stream and restores it in the destructor
struct StreamStateSaver final {
  explicit StreamStateSaver(std::ios &s)  // NOLINT(runtime/references)
      : ref(s) {
    state.copyfmt(s);
  }

  ~StreamStateSaver() {
    ref.copyfmt(state);
  }

  std::ios &ref;
  std::ios state{nullptr};
};

template <typename It>
inline void
// NOLINTNEXTLINE(runtime/references)
dump_hexstring(std::ostream &out , It first, It last, bool upper_case = false) {
  static_assert(
      std::is_same<typename std::iterator_traits<It>::value_type, char>::value,
      "Iterator needs to dereference to char");
  utils::StreamStateSaver state_saver(out);
  for (auto it = first; it < last; ++it) {
    out << std::setw(2) << std::setfill('0') << std::hex
        << (upper_case ? std::uppercase : std::nouppercase)
        << static_cast<int>(static_cast<unsigned char>(*it));
  }
}

inline void
// NOLINTNEXTLINE(runtime/references)
dump_hexstring(std::ostream &out, const std::string &s,
               bool upper_case = false) {
  dump_hexstring(out, s.begin(), s.end(), upper_case);
}

}  // namespace utils

}  // namespace bm

#endif  // BM_SIM_UTILS_H_
