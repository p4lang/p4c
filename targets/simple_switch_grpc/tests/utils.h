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

#ifndef SIMPLE_SWITCH_GRPC_TESTS_UTILS_H_
#define SIMPLE_SWITCH_GRPC_TESTS_UTILS_H_

#include <p4/config/v1/p4info.grpc.pb.h>

#include <string>

namespace sswitch_grpc {

namespace testing {

int get_table_id(const p4::config::v1::P4Info &p4info,
                 const std::string &t_name);

int get_action_id(const p4::config::v1::P4Info &p4info,
                  const std::string &a_name);

int get_mf_id(const p4::config::v1::P4Info &p4info,
              const std::string &t_name, const std::string &mf_name);

int get_param_id(const p4::config::v1::P4Info &p4info,
                 const std::string &a_name, const std::string &param_name);

p4::config::v1::P4Info parse_p4info(const char *path);

}  // namespace testing

}  // namespace sswitch_grpc

#endif  // SIMPLE_SWITCH_GRPC_TESTS_UTILS_H_
