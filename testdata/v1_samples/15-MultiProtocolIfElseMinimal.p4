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

header_type vlan_tag_t {
    fields {
        pcp : 3;
        cfi : 1;
        vid : 12;
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

header ethernet_t ethernet;
header vlan_tag_t vlan_tag;
header ipv4_t     ipv4;

parser start {
    extract(ethernet);
    return select(latest.etherType) {
        0x8100, 0x9100 : parse_vlan_tag;
        0x0800         : parse_ipv4;
        default        : ingress;
    }
}

parser parse_vlan_tag {
    extract(vlan_tag);
    return select(latest.etherType) {
        0x0800         : parse_ipv4;
        default        : ingress;
    }
}

parser parse_ipv4 {
    extract(ipv4);
    return ingress;
}

header_type ingress_metadata_t {
    fields {
        drop : 1;
        egress_port : 8;
        packet_type : 4;
    }
} 

metadata ingress_metadata_t ing_metadata;

action nop() {
}

action set_egress_port(egress_port) {
    modify_field(ing_metadata.egress_port, egress_port);
}

table ipv4_match {
    reads {
        ipv4.srcAddr : exact;
    }
    actions {
        nop;
        set_egress_port;
    }
}

table l2_match {
    reads {
        ethernet.srcAddr : exact;
    }
    actions {
        nop;
        set_egress_port;
    }
}

control ingress {
    if (ethernet.etherType == 0x0800) {
        apply(ipv4_match);
    } else {
        apply(l2_match);
    }
}

control egress {
}
