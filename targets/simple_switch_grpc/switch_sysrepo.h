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

#ifndef SIMPLE_SWITCH_GRPC_SWITCH_SYSREPO_H_
#define SIMPLE_SWITCH_GRPC_SWITCH_SYSREPO_H_

#include <set>
#include <string>

extern "C" {
#include "sysrepo.h"
#include "sysrepo/values.h"
#include "sysrepo/xpath.h"
}

namespace bm {

class DevMgr;

}  // namespace bm

namespace sswitch_grpc {

class SysrepoSubscriber {
 public:
  ~SysrepoSubscriber();
  bool start();

 private:
  sr_conn_ctx_t *connection{NULL};
  sr_session_ctx_t *session{NULL};
  sr_subscription_ctx_t *subscription{NULL};
  static constexpr const char *const app_name = "subscriber";
  static constexpr const char *const module_names[] =
      {"openconfig-interfaces", "openconfig-platform"};
};

class SysrepoTest {
 public:
  explicit SysrepoTest(const bm::DevMgr *dev_mgr);
  ~SysrepoTest();
  bool connect();
  void refresh_ports();
  void provide_oper_state(sr_val_t **values, size_t *values_cnt);

 private:
  void add_iface(const std::string &name);

  const bm::DevMgr *dev_mgr;  // non-owning pointer
  sr_conn_ctx_t *connection{NULL};
  sr_session_ctx_t *session{NULL};
  sr_subscription_ctx_t *subscription{NULL};
  static constexpr const char *const app_name = "test";
};

}  // namespace sswitch_grpc

#endif  // SIMPLE_SWITCH_GRPC_SWITCH_SYSREPO_H_
