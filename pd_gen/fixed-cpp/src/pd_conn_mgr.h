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

#ifndef _P4_PD_CONN_MGR_H_
#define _P4_PD_CONN_MGR_H_

#include "Standard.h"
#include "SimplePreLAG.h"

using namespace  ::bm_runtime::standard;
using namespace  ::bm_runtime::simple_pre_lag;

typedef StandardClient Client;
typedef SimplePreLAGClient McClient;

typedef struct pd_conn_mgr_s pd_conn_mgr_t;

pd_conn_mgr_t *pd_conn_mgr_create();
void pd_conn_mgr_destroy(pd_conn_mgr_t *conn_mgr_state);

Client *pd_conn_mgr_client(pd_conn_mgr_t *, int dev_id);
McClient *pd_conn_mgr_mc_client(pd_conn_mgr_t *, int dev_id);

int pd_conn_mgr_client_init(pd_conn_mgr_t *, int dev_id, int thrift_port_num);
int pd_conn_mgr_client_close(pd_conn_mgr_t *, int dev_id);

#endif
