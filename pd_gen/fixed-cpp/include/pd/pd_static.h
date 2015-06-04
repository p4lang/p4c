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

#ifndef _P4_PD_STATIC_H_
#define _P4_PD_STATIC_H_

#include "pd/pd_common.h"

#ifdef __cplusplus
extern "C" {
#endif

p4_pd_status_t
p4_pd_init(void);

void
p4_pd_cleanup(void);

p4_pd_status_t
p4_pd_client_init(p4_pd_sess_hdl_t *sess_hdl, uint32_t max_txn_size);

p4_pd_status_t
p4_pd_client_cleanup(p4_pd_sess_hdl_t sess_hdl);

p4_pd_status_t
p4_pd_begin_txn(p4_pd_sess_hdl_t shdl, bool isAtomic, bool isHighPri);

p4_pd_status_t
p4_pd_verify_txn(p4_pd_sess_hdl_t shdl);

p4_pd_status_t
p4_pd_abort_txn(p4_pd_sess_hdl_t shdl);

p4_pd_status_t
p4_pd_commit_txn(p4_pd_sess_hdl_t shdl, bool hwSynchronous, bool *sendRsp);

p4_pd_status_t
p4_pd_complete_operations(p4_pd_sess_hdl_t shdl);

p4_pd_status_t
p4_pd_load_new_config(p4_pd_sess_hdl_t shdl, uint8_t dev_id,
		      const char *config_str);

p4_pd_status_t
p4_pd_swap_configs(p4_pd_sess_hdl_t shdl, uint8_t dev_id);

#ifdef __cplusplus
}
#endif

#endif
