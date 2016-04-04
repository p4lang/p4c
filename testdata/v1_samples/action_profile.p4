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

header_type ipv4_t {
    fields {
        version : 4;
        ihl : 4;
        diffserv : 8;
        totalLen : 16;
        identification : 16;
        flags : 3;
        fragOffset : 13;
        ttl : 8;
        protocol : 8;
        hdrChecksum : 16;
        srcAddr : 32;
        dstAddr: 32;
    }
}

header_type intrinsic_metadata_t {
    fields {
        mcast_grp : 4;
        egress_rid : 4;
        mcast_hash : 16;
        lf_field_list: 32;
    }
}

parser start {
    return parse_ethernet;
}

header ethernet_t ethernet;
header ipv4_t ipv4;
metadata intrinsic_metadata_t intrinsic_metadata;

#define ETHERTYPE_IPV4 0x0800

parser parse_ethernet {
    extract(ethernet);
    return select(ethernet.etherType) {
        ETHERTYPE_IPV4: parse_ipv4;
        default: ingress;
    }
}

parser parse_ipv4 {
    extract(ipv4);
    return ingress;
}

action _drop() {
    drop();
}

action _nop() {
}

field_list l3_hash_fields {
    ipv4.srcAddr;
    ipv4.protocol;
}

field_list_calculation ecmp_hash {
    input {
        l3_hash_fields;
    }
    algorithm : bmv2_hash;
    output_width : 16;
}

action_selector ecmp_selector {
    selection_key : ecmp_hash;
}

action set_ecmp_nexthop(port) {
    modify_field(standard_metadata.egress_spec, port);
}

action_profile ecmp_action_profile {
    actions {
        _nop;
        set_ecmp_nexthop;
    }
    size : 64;
    dynamic_action_selection : ecmp_selector;
}

table ecmp_group {
    reads {
        ipv4.dstAddr : exact;
    }
    action_profile: ecmp_action_profile;
    size : 1024;
}

control ingress {
    apply(ecmp_group);
}

control egress {
}
