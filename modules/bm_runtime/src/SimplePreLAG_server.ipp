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

#include "SimplePreLAG.h"

#include <bm_sim/simple_pre_lag.h>

namespace bm_runtime { namespace simple_pre_lag {

class SimplePreLAGHandler : virtual public SimplePreLAGIf {
public:
  SimplePreLAGHandler(Switch *sw) 
    : switch_(sw) {
    pre = sw->get_component<McSimplePreLAG>();
    assert(pre != nullptr);
  }

  // TODO Krishna

private:
  Switch *switch_{nullptr};
  std::shared_ptr<McSimplePreLAG> pre{nullptr};
};

} }
