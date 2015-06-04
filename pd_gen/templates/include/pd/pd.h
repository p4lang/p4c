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

#ifndef _P4_PD_H_
#define _P4_PD_H_

#include "pd/pd_static.h"

#ifdef __cplusplus
extern "C" {
#endif

p4_pd_status_t ${pd_prefix}init(p4_pd_sess_hdl_t sess_hdl,
				const char *learning_addr);

p4_pd_status_t ${pd_prefix}assign_device(p4_pd_sess_hdl_t sess_hdl,
					 int dev_id, int rpc_port_num);

p4_pd_status_t ${pd_prefix}remove_device(p4_pd_sess_hdl_t sess_hdl, int dev_id);

#ifdef __cplusplus
}
#endif

#endif
