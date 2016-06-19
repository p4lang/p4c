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

#include <bm/Standard.h>

#include <bm/pdfixed/pd_swap.h>
#include <bm/pdfixed/int/pd_conn_mgr.h>

#include <iostream>
#include <string>

using ::bm_runtime::standard::StandardClient;
using ::bm_runtime::standard::InvalidSwapOperation;

extern PdConnMgr *conn_mgr_state;

namespace {

Client<StandardClient> client(int device) {
  return conn_mgr_state->get<StandardClient>(device);
}

const char *get_swap_error_name(int code) {
  using ::bm_runtime::standard::_SwapOperationErrorCode_VALUES_TO_NAMES;
  return _SwapOperationErrorCode_VALUES_TO_NAMES.find(code)->second;
}

}  // namespace

extern "C" {

p4_pd_status_t
p4_pd_load_new_config(p4_pd_sess_hdl_t shdl, uint8_t dev_id,
                      const char *config_str) {
  (void) shdl;
  try {
    client(dev_id).c->bm_load_new_config(std::string(config_str));
  } catch(InvalidSwapOperation &iso) {
    const char *what = get_swap_error_name(iso.code);
    std::cout << "Invalid swap operation (" << iso.code << "): "
              << what << std::endl;
    return iso.code;
  }
  return 0;
}

p4_pd_status_t
p4_pd_swap_configs(p4_pd_sess_hdl_t shdl, uint8_t dev_id) {
  (void) shdl;
  try {
    client(dev_id).c->bm_swap_configs();
  } catch(InvalidSwapOperation &iso) {
    const char *what = get_swap_error_name(iso.code);
    std::cout << "Invalid swap operation (" << iso.code << "): "
              << what << std::endl;
    return iso.code;
  }
  return 0;
}

}
