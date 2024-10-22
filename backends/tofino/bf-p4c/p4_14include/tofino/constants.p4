/**
 * Copyright (C) 2024 Intel Corporation
 * 
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License.  You may obtain a copy
 * of the License at
 * 
 * http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software distributed
 * under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 * CONDITIONS OF ANY KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations under the License.
 * 
 * 
 * SPDX-License-Identifier: Apache-2.0
 */


// This file is to be kept in precise sync with constants.py

#ifndef TOFINO_LIB_CONSTANTS
#define TOFINO_LIB_CONSTANTS 1

/////////////////////////////////////////////////////////////
// Parser hardware error codes
#define PARSER_ERROR_OK             0x0000
#define PARSER_ERROR_NO_TCAM        0x0001
#define PARSER_ERROR_PARTIAL_HDR    0x0002
#define PARSER_ERROR_CTR_RANGE      0x0004
#define PARSER_ERROR_TIMEOUT_USER   0x0008
#define PARSER_ERROR_TIMEOUT_HW     0x0010
#define PARSER_ERROR_SRC_EXT        0x0020
#define PARSER_ERROR_DST_CONT       0x0040
#define PARSER_ERROR_PHV_OWNER      0x0080
#define PARSER_ERROR_MULTIWRITE     0x0100
#define PARSER_ERROR_ARAM_SBE       0x0200
#define PARSER_ERROR_ARAM_MBE       0x0400
#define PARSER_ERROR_FCS            0x0800
#define PARSER_ERROR_CSUM           0x1000

#define PARSER_ERROR_ARRAY_OOB      0xC000
/////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////
// Digest receivers
#define FLOW_LRN_DIGEST_RCVR    0

#define RECIRCULATE_DIGEST_RCVR 90
#define RESUBMIT_DIGEST_RCVR    91
#define CLONE_I2I_DIGEST_RCVR   92
#define CLONE_E2I_DIGEST_RCVR   93
#define CLONE_I2E_DIGEST_RCVR   94
#define CLONE_E2E_DIGEST_RCVR   95
/////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////
// Clone soruces
// (to be used with eg_intr_md_from_parser_aux.clone_src)
#define NOT_CLONED              0
#define CLONED_FROM_INGRESS     1
#define CLONED_FROM_EGRESS      3
#define COALESCED               5
/////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////
// Default priorities
#define PARSER_DEF_PRI     0
#define PARSER_THRESH_PRI  3

#endif
