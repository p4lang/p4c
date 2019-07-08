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

header_type ethernet_t {
    fields {
        dstAddr : 48;
        srcAddr : 48;
        etherType : 16;
    }
}

header_type intrinsic_metadata_t {
    fields {
        mcast_grp : 4;
        egress_rid : 4;
    }
}

header_type mymeta_t {
    fields {
        f1 : 8;
    }
}

header ethernet_t ethernet;
metadata intrinsic_metadata_t intrinsic_metadata;
metadata mymeta_t mymeta;

parser start {
    return parse_ethernet;
}

parser parse_ethernet {
    extract(ethernet);
    return ingress;
}

action _drop() {
    drop();
}

action _nop() {
}

action set_port(port) {
    modify_field(standard_metadata.egress_spec, port);
}

field_list resubmit_FL {
    standard_metadata;
    mymeta;
}

action _resubmit() {
    modify_field(mymeta.f1, 1);
    resubmit(resubmit_FL);
}

table t_ingress_1 {
    reads {
        mymeta.f1 : exact;
    }
    actions {
        _nop; set_port;
    }
    size : 128;
}

table t_ingress_2 {
    reads {
        mymeta.f1 : exact;
    }
    actions {
        _nop; _resubmit;
    }
    size : 128;
}

control ingress {
    apply(t_ingress_1);
    apply(t_ingress_2);
}

control egress {
}
