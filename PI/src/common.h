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

#ifndef SRC_COMMON_H_
#define SRC_COMMON_H_

#include <bm/bm_sim/match_error_codes.h>

#include <PI/int/pi_int.h>
#include <PI/p4info.h>
#include <PI/pi.h>

#include <exception>
#include <string>
#include <unordered_map>
#include <vector>

namespace bm {

class SwitchWContexts;

}  // namespace bm

namespace pibmv2 {

typedef struct {
  int assigned;
  const pi_p4info_t *p4info;
} device_info_t;

extern device_info_t device_info_state;

extern bm::SwitchWContexts *switch_;

static inline device_info_t *get_device_info(size_t dev_id) {
  (void) dev_id;
  return &device_info_state;
}

static inline device_info_t *get_device_info() {
  return &device_info_state;
}

static inline pi_status_t convert_error_code(bm::MatchErrorCode error_code) {
  return static_cast<pi_status_t>(
      PI_STATUS_TARGET_ERROR + static_cast<int>(error_code));
}

struct IndirectHMgr {
  static pi_indirect_handle_t make_grp_h(pi_indirect_handle_t h) {
    return h | grp_prefix;
  }

  static bool is_grp_h(pi_indirect_handle_t h) { return h & grp_prefix; }

  static pi_indirect_handle_t clear_grp_h(pi_indirect_handle_t h) {
    return h & (~grp_prefix);
  }

  static constexpr pi_indirect_handle_t grp_prefix =
      (1ull << (sizeof(pi_indirect_handle_t) * 8 - 1));
};

class Buffer {
 public:
  Buffer();

  char *extend(size_t s);

  char *copy() const;

  size_t size() const;

 private:
  std::vector<char> data{};
};

}  // namespace pibmv2

#endif  // SRC_COMMON_H_
