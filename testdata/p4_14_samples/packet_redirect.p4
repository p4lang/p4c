/*
Copyright 2013-present Barefoot Networks, Inc.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

header_type hdrA_t {
    fields {
        f1 : 8;
        f2 : 8;
    }
}

header hdrA_t hdrA;

header_type metaA_t {
    fields {
        f1 : 8;
        f2 : 8;
    }
}

header_type metaB_t {
    fields {
        f1 : 8;
        f2 : 8;
    }
}

metadata metaA_t metaA;
metadata metaB_t metaB;

header_type intrinsic_metadata_t {
    fields {
        mcast_grp : 16;
        egress_rid : 4;
        ingress_global_timestamp : 64;
    }
}

metadata intrinsic_metadata_t intrinsic_metadata;

parser start {
    extract(hdrA);
    return ingress;
}

action _nop() {
}

field_list redirect_FL {
    standard_metadata;
    metaA;
}

action _resubmit() {
    resubmit(redirect_FL);
}

action _recirculate() {
    recirculate(redirect_FL);
}

action _multicast(mgrp) {
    modify_field(intrinsic_metadata.mcast_grp, mgrp);
}

action _clone_i2e(mirror_id) {
    clone_ingress_pkt_to_egress(mirror_id, redirect_FL);
}

action _clone_e2e(mirror_id) {
    clone_egress_pkt_to_egress(mirror_id, redirect_FL);
}

action _set_port(port) {
    modify_field(standard_metadata.egress_spec, port);
    modify_field(metaA.f1, 1);
}

table t_ingress_1 {
    reads {
        hdrA.f1 : exact;
        metaA.f1 : exact;
    }
    actions {
        _nop; _set_port; _multicast;
    }
    size : 128;
}

table t_ingress_2 {
    reads {
        hdrA.f1 : exact;
        standard_metadata.instance_type : ternary;
    }
    actions {
        _nop; _resubmit; _clone_i2e;
    }
    size : 128;
}

table t_egress {
    reads {
        hdrA.f1 : exact;
        standard_metadata.instance_type : ternary;
    }
    actions {
        _nop; _recirculate; _clone_e2e;
    }
    size : 128;
}

control ingress {
    apply(t_ingress_1);
    apply(t_ingress_2);
}

control egress {
    apply(t_egress);
}
