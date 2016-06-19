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
#include <bm/SimplePreLAG.h>
#include <bm/SimpleSwitch.h>

#include <bm/pdfixed/pd_static.h>
#include <bm/pdfixed/int/pd_conn_mgr.h>

using ::bm_runtime::standard::StandardClient;
using ::bm_runtime::simple_pre_lag::SimplePreLAGClient;
using ::sswitch_runtime::SimpleSwitchClient;

static uint64_t session_hdl = 0;

PdConnMgr *conn_mgr_state;

extern "C" {

p4_pd_status_t
p4_pd_init(void) {
  using ConnMgr = detail::PdConnMgr_<StandardClient, SimplePreLAGClient,
                                     SimpleSwitchClient>;
  conn_mgr_state = new ConnMgr("standard", "simple_pre_lag", "simple_switch");
  return 0;
}

void
p4_pd_cleanup(void) {
  delete conn_mgr_state;
}

p4_pd_status_t
p4_pd_client_init(p4_pd_sess_hdl_t *sess_hdl) {
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
p4_pd_commit_txn(p4_pd_sess_hdl_t shdl, bool hwSynchronous) {
  (void) shdl; (void) hwSynchronous;
  return 0;
}

p4_pd_status_t
p4_pd_complete_operations(p4_pd_sess_hdl_t shdl) {
  (void) shdl;
  return 0;
}

}
