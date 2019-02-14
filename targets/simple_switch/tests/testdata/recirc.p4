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

header_type intrinsic_metadata_t {
    fields {
        mcast_grp : 4;
        egress_rid : 4;
        mcast_hash : 16;
        lf_field_list: 32;
        ingress_global_timestamp : 64;
        resubmit_flag : 16;
        recirculate_flag : 16;
    }
}

metadata intrinsic_metadata_t intrinsic_metadata;

header_type hdrA_t {
    fields {
        f1 : 8;
    }
}

header hdrA_t hdrA1;
header hdrA_t hdrA2;

parser start {
    extract(hdrA1);
    return select(hdrA1.f1) {
        0x00: ingress;
        default: more;
    }
}

parser more {
    extract(hdrA2);
    set_metadata(hdrA2.f1, 0xab);
    return ingress;
}

field_list recirc_FL {
    standard_metadata;
}

action loopback() {
    modify_field(standard_metadata.egress_spec, standard_metadata.ingress_port);
}

table t_loopback {
    actions { loopback; }
    default_action: loopback();
}

action recirc() {
    modify_field(hdrA1.f1, 0x01);
    add_header(hdrA2);
    recirculate(recirc_FL);
}

table t_recirc {
    actions { recirc; }
    default_action: recirc();
}

control ingress {
    apply(t_loopback);
}

control egress {
    if (hdrA1.f1 == 0) {
        apply(t_recirc);
    }
}
