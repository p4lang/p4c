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

#include <bm/pdfixed/pd_static.h>
#include <bm/pdfixed/int/pd_conn_mgr.h>

#include <cassert>
#include <iostream>

static uint64_t session_hdl = 0;

pd_conn_mgr_t *conn_mgr_state;

extern "C" {

p4_pd_status_t
p4_pd_init(void) {
  conn_mgr_state = pd_conn_mgr_create();
  return 0;
}

void
p4_pd_cleanup(void) {
  
}

p4_pd_status_t
p4_pd_client_init(p4_pd_sess_hdl_t *sess_hdl, uint32_t max_txn_size) {
  (void) max_txn_size;
  *sess_hdl = session_hdl++;
  return 0;
}

p4_pd_status_t
p4_pd_client_cleanup(p4_pd_sess_hdl_t sess_hdl) {
  (void) sess_hdl;
  return 0;
}

p4_pd_status_t
p4_pd_begin_txn(p4_pd_sess_hdl_t shdl, bool isAtomic, bool isHighPri) {
  (void) shdl; (void) isAtomic; (void) isHighPri;
  return 0;
}

p4_pd_status_t
p4_pd_verify_txn(p4_pd_sess_hdl_t shdl) {
  (void) shdl;
  return 0;
}

p4_pd_status_t
p4_pd_abort_txn(p4_pd_sess_hdl_t shdl) {
  (void) shdl;
  return 0;
}

p4_pd_status_t
p4_pd_commit_txn(p4_pd_sess_hdl_t shdl, bool hwSynchronous, bool *sendRsp) {
  (void) shdl; (void) hwSynchronous; (void) sendRsp;
  return 0;
}

p4_pd_status_t
p4_pd_complete_operations(p4_pd_sess_hdl_t shdl) {
  (void) shdl;
  return 0;
}

p4_pd_status_t
p4_pd_load_new_config(p4_pd_sess_hdl_t shdl, uint8_t dev_id,
		      const char *config_str) {
  (void) shdl;
  auto client = pd_conn_mgr_client(conn_mgr_state, dev_id);
  try {
    client.c->bm_load_new_config(std::string(config_str));
  } catch(InvalidSwapOperation &iso) {
    const char *what =
      _SwapOperationErrorCode_VALUES_TO_NAMES.find(iso.code)->second;
    std::cout << "Invalid swap operation (" << iso.code << "): "
	      << what << std::endl;
    return iso.code;
  }
  return 0;
}

p4_pd_status_t
p4_pd_swap_configs(p4_pd_sess_hdl_t shdl, uint8_t dev_id) {
  (void) shdl;
  auto client = pd_conn_mgr_client(conn_mgr_state, dev_id);
  try {
    client.c->bm_swap_configs();
  } catch(InvalidSwapOperation &iso) {
    const char *what =
      _SwapOperationErrorCode_VALUES_TO_NAMES.find(iso.code)->second;
    std::cout << "Invalid swap operation (" << iso.code << "): "
	      << what << std::endl;
    return iso.code;
  }
  return 0;
}

}
