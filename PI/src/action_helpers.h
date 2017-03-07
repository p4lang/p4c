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

#ifndef SRC_ACTION_HELPERS_H_
#define SRC_ACTION_HELPERS_H_

#include <bm/bm_sim/actions.h>

#include <PI/pi.h>

#include <string>
#include <vector>

namespace pibmv2 {

bm::ActionData build_action_data(const pi_action_data_t *action_data,
                                 const pi_p4info_t *p4info);

char *dump_action_data(const pi_p4info_t *p4info, char *data,
                       pi_p4_id_t action_id, const bm::ActionData &action_data);

}  // namespace pibmv2

#endif  // SRC_ACTION_HELPERS_H_
