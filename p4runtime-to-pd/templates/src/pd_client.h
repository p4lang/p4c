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

#ifndef _P4_SRC_PD_CLIENT_H_
#define _P4_SRC_PD_CLIENT_H_

#include <bm/Standard.h>

#include <bm/pdfixed/int/pd_conn_mgr.h>

using namespace  ::bm_runtime::standard;

extern PdConnMgr *conn_mgr_state;

inline Client<StandardClient> pd_client(int device) {
  return conn_mgr_state->get<StandardClient>(device);
}

#endif
