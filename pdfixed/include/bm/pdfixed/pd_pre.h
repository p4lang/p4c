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

#ifndef BM_PDFIXED_PD_PRE_H_
#define BM_PDFIXED_PD_PRE_H_

#include <bm/pdfixed/pd_common.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PRE_PORTS_MAX 256
#define PRE_LAG_MAX 256

typedef uint16_t mgrp_id_t;
typedef uint16_t mgrp_rid_t;
typedef uint16_t mgrp_lag_id_t;
typedef uint16_t mgrp_port_id_t;

p4_pd_status_t
p4_pd_mc_create_session(p4_pd_sess_hdl_t *sess_hdl);

p4_pd_status_t
p4_pd_mc_delete_session(p4_pd_sess_hdl_t sess_hdl);

p4_pd_status_t
p4_pd_mc_complete_operations(p4_pd_sess_hdl_t sess_hdl);

p4_pd_status_t
p4_pd_mc_mgrp_create(p4_pd_sess_hdl_t session,
                     int device,
                     mgrp_id_t mgid,
                     p4_pd_entry_hdl_t *mgrp_hdl);

p4_pd_status_t
p4_pd_mc_mgrp_destroy(p4_pd_sess_hdl_t session,
                      int device,
                      p4_pd_entry_hdl_t mgrp_hdl);

p4_pd_status_t
p4_pd_mc_node_create(p4_pd_sess_hdl_t session,
                     int device,
                     mgrp_rid_t rid,
                     const uint8_t *port_map,
                     const uint8_t *lag_map,
                     p4_pd_entry_hdl_t *node_hdl);

p4_pd_status_t
p4_pd_mc_node_destroy(p4_pd_sess_hdl_t session,
                      int device,
                      p4_pd_entry_hdl_t node_hdl);

p4_pd_status_t
p4_pd_mc_node_update(p4_pd_sess_hdl_t session,
                     int device,
                     p4_pd_entry_hdl_t node_hdl,
                     const uint8_t *port_map,
                     const uint8_t *lag_map);

p4_pd_status_t
p4_pd_mc_associate_node(p4_pd_sess_hdl_t session,
                        int device,
                        p4_pd_entry_hdl_t mgrp_hdl,
                        p4_pd_entry_hdl_t node_hdl,
                        uint16_t xid, bool xid_valid);

p4_pd_status_t
p4_pd_mc_dissociate_node(p4_pd_sess_hdl_t session,
                         int device,
                         p4_pd_entry_hdl_t mgrp_hdl,
                         p4_pd_entry_hdl_t node_hdl);

p4_pd_status_t
p4_pd_mc_set_lag_membership(p4_pd_sess_hdl_t session,
                            int device,
                            mgrp_lag_id_t lag_id,
                            const uint8_t *port_map);

p4_pd_status_t
p4_pd_mc_ecmp_create(p4_pd_sess_hdl_t session,
                     int device,
                     p4_pd_entry_hdl_t *ecmp_hdl);

p4_pd_status_t
p4_pd_mc_ecmp_destroy(p4_pd_sess_hdl_t session,
                      int device,
                      p4_pd_entry_hdl_t ecmp_hdl);

p4_pd_status_t
p4_pd_mc_ecmp_mbr_add(p4_pd_sess_hdl_t session,
                      int device,
                      p4_pd_entry_hdl_t ecmp_hdl,
                      p4_pd_entry_hdl_t l1_hdl);

p4_pd_status_t
p4_pd_mc_ecmp_mbr_rem(p4_pd_sess_hdl_t session,
                      int device,
                      p4_pd_entry_hdl_t ecmp_hdl,
                      p4_pd_entry_hdl_t l1_hdl);

p4_pd_status_t
p4_pd_mc_associate_ecmp(p4_pd_sess_hdl_t session,
                        int device,
                        p4_pd_entry_hdl_t grp_hdl,
                        p4_pd_entry_hdl_t ecmp_hdl,
                        uint16_t xid, bool xid_valid);

p4_pd_status_t
p4_pd_mc_dissociate_ecmp(p4_pd_sess_hdl_t session,
                         int device,
                         p4_pd_entry_hdl_t grp_hdl,
                         p4_pd_entry_hdl_t ecmp_hdl);

p4_pd_status_t
p4_pd_mc_update_port_prune_table(p4_pd_sess_hdl_t session,
                                 int device,
                                 uint16_t yid, uint8_t *port_map);

#ifdef __cplusplus
}
#endif

#endif  // BM_PDFIXED_PD_PRE_H_
