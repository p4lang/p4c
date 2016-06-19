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
 * Srikrishna Gopu (krishna@barefootnetworks.com)
 * Antonin Bas (antonin@barefootnetworks.com)
 *
 */

#include <bm/SimplePreLAG.h>

#include <bm/pdfixed/pd_pre.h>
#include <bm/pdfixed/pd_static.h>
#include <bm/pdfixed/int/pd_conn_mgr.h>

#include <string>

using ::bm_runtime::simple_pre_lag::SimplePreLAGClient;

extern PdConnMgr *conn_mgr_state;

namespace {

Client<SimplePreLAGClient> client(int device) {
  return conn_mgr_state->get<SimplePreLAGClient>(device);
}

std::string convert_map(const uint8_t *input, const size_t size) {
  std::string output(size, '0');
  for (size_t i = 0; i < size; i++) {
    uint8_t byte = input[i / 8];
    int offset = i % 8;
    if (byte & (1 << offset)) {
      output[size - 1 - i] = '1';
    }
  }
  return output;
}

}  // namespace

extern "C" {

p4_pd_status_t
p4_pd_mc_create_session(p4_pd_sess_hdl_t *sess_hdl) {
  (void) sess_hdl;
  return 0;
}

p4_pd_status_t
p4_pd_mc_delete_session(p4_pd_sess_hdl_t sess_hdl) {
  (void) sess_hdl;
  return 0;
}

p4_pd_status_t
p4_pd_mc_complete_operations(p4_pd_sess_hdl_t sess_hdl) {
  (void) sess_hdl;
  return 0;
}

p4_pd_status_t
p4_pd_mc_mgrp_create(p4_pd_sess_hdl_t session, int device,
                     mgrp_id_t mgid, p4_pd_entry_hdl_t *mgrp_hdl) {
  (void) session;
  *mgrp_hdl = client(device).c->bm_mc_mgrp_create(0, mgid);
  return 0;  // TODO(unknown)
}

p4_pd_status_t
p4_pd_mc_mgrp_destroy(p4_pd_sess_hdl_t session, int device,
                      p4_pd_entry_hdl_t mgrp_hdl) {
  (void) session;
  client(device).c->bm_mc_mgrp_destroy(0, mgrp_hdl);
  return 0;  // TODO(unknown)
}

p4_pd_status_t
p4_pd_mc_node_create(p4_pd_sess_hdl_t session, int device,
                     mgrp_rid_t rid, const uint8_t *port_map,
                     const uint8_t *lag_map, p4_pd_entry_hdl_t *node_hdl) {
  (void) session;
  std::string port_map_ = convert_map(port_map, PRE_PORTS_MAX);
  std::string lag_map_ = convert_map(lag_map, PRE_LAG_MAX);
  *node_hdl = client(device).c->bm_mc_node_create(0, rid, port_map_, lag_map_);
  return 0;  // TODO(unknown)
}

p4_pd_status_t
p4_pd_mc_associate_node(p4_pd_sess_hdl_t session, int device,
                        p4_pd_entry_hdl_t mgrp_hdl, p4_pd_entry_hdl_t hdl,
                        uint16_t xid, bool xid_valid) {
  (void) session; (void) xid; (void) xid_valid;
  client(device).c->bm_mc_node_associate(0, mgrp_hdl, hdl);
  return 0;  // TODO(unknown)
}

p4_pd_status_t
p4_pd_mc_dissociate_node(p4_pd_sess_hdl_t session, int device,
                         p4_pd_entry_hdl_t mgrp_hdl,
                         p4_pd_entry_hdl_t node_hdl) {
  // TODO(unknown): needed ?
  (void) session; (void) device; (void) mgrp_hdl; (void) node_hdl;
  return 0;
}

p4_pd_status_t
p4_pd_mc_node_destroy(p4_pd_sess_hdl_t session, int device,
                      p4_pd_entry_hdl_t node_hdl) {
  (void) session;
  client(device).c->bm_mc_node_destroy(0, node_hdl);
  return 0;  // TODO(unknown)
}

p4_pd_status_t
p4_pd_mc_node_update(p4_pd_sess_hdl_t session, int device,
                     p4_pd_entry_hdl_t node_hdl, const uint8_t *port_map,
                     const uint8_t *lag_map) {
  (void) session;
  std::string port_map_ = convert_map(port_map, PRE_PORTS_MAX);
  std::string lag_map_ = convert_map(lag_map, PRE_LAG_MAX);
  client(device).c->bm_mc_node_update(0, node_hdl, port_map_, lag_map_);
  return 0;  // TODO(unknown)
}

p4_pd_status_t
p4_pd_mc_set_lag_membership(p4_pd_sess_hdl_t session, int device,
                            mgrp_lag_id_t lag_id, const uint8_t *port_map) {
  (void) session;
  std::string port_map_ = convert_map(port_map, PRE_PORTS_MAX);
  client(device).c->bm_mc_set_lag_membership(0, lag_id, port_map_);
  return 0;  // TODO(unknown)
}

p4_pd_status_t
p4_pd_mc_ecmp_create(p4_pd_sess_hdl_t session,
                     int device,
                     p4_pd_entry_hdl_t *ecmp_hdl) {
  (void) session; (void) device; (void) ecmp_hdl;
  return 0;
}

p4_pd_status_t
p4_pd_mc_ecmp_destroy(p4_pd_sess_hdl_t session,
                      int device,
                      p4_pd_entry_hdl_t ecmp_hdl) {
  (void) session; (void) device; (void) ecmp_hdl;
  return 0;
}

p4_pd_status_t
p4_pd_mc_ecmp_mbr_add(p4_pd_sess_hdl_t session,
                      int device,
                      p4_pd_entry_hdl_t ecmp_hdl,
                      p4_pd_entry_hdl_t l1_hdl) {
  (void) session; (void) device; (void) ecmp_hdl; (void) l1_hdl;
  return 0;
}

p4_pd_status_t
p4_pd_mc_ecmp_mbr_rem(p4_pd_sess_hdl_t session,
                      int device,
                      p4_pd_entry_hdl_t ecmp_hdl,
                      p4_pd_entry_hdl_t l1_hdl) {
  (void) session; (void) device; (void) ecmp_hdl; (void) l1_hdl;
  return 0;
}

p4_pd_status_t
p4_pd_mc_associate_ecmp(p4_pd_sess_hdl_t session,
                        int device,
                        p4_pd_entry_hdl_t grp_hdl,
                        p4_pd_entry_hdl_t ecmp_hdl,
                        uint16_t xid, bool xid_valid) {
  (void) session; (void) device; (void) grp_hdl; (void) ecmp_hdl;
  (void) xid; (void) xid_valid;
  return 0;
}

p4_pd_status_t
p4_pd_mc_dissociate_ecmp(p4_pd_sess_hdl_t session,
                         int device,
                         p4_pd_entry_hdl_t grp_hdl,
                         p4_pd_entry_hdl_t ecmp_hdl) {
  (void) session; (void) device; (void) grp_hdl; (void) ecmp_hdl;
  return 0;
}

p4_pd_status_t
p4_pd_mc_update_port_prune_table(p4_pd_sess_hdl_t session,
                                 int device,
                                 uint16_t yid, uint8_t *port_map) {
  (void) session; (void) device; (void) yid; (void) port_map;
  return 0;
}

}
