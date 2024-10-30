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


#ifndef TOFINO_LIB_PRIMITIVES
#define TOFINO_LIB_PRIMITIVES 1

/////////////////////////////////////////////////////////////
// Primitive aliases

#define clone_i2e clone_ingress_pkt_to_egress
#define clone_e2e clone_egress_pkt_to_egress

action deflect_on_drop(enable_dod) {
    modify_field(ig_intr_md_for_tm.deflect_on_drop, enable_dod);
}

#define _ingress_global_tstamp_     ig_intr_md_from_parser_aux.ingress_global_tstamp

#define _parser_counter_ ig_prsr_ctrl.parser_counter

#define pkt_is_mirrored (eg_intr_md_from_parser_aux.clone_src != NOT_CLONED)
#define pkt_is_not_mirrored (eg_intr_md_from_parser_aux.clone_src == NOT_CLONED)

#define pkt_is_i2e_mirrored (eg_intr_md_from_parser_aux.clone_src == CLONED_FROM_INGRESS)
#define pkt_is_e2e_mirrored (eg_intr_md_from_parser_aux.clone_src == CLONED_FROM_EGRESS)
#define pkt_is_coalesced (eg_intr_md_from_parser_aux.clone_src == COALESCED)
#define pkt_is_not_coalesced (eg_intr_md_from_parser_aux.clone_src != COALESCED)

#endif
