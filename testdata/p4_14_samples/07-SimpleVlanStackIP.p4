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
        dstAddr   : 48;
        srcAddr   : 48;
        ethertype : 16;
    }
}

header_type vlan_tag_t {
    fields {
        pcp       : 3;
        cfi       : 1;
        vlan_id   : 12;
        ethertype : 16;
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

#define N_TAGS 2

header ethernet_t ethernet;
header vlan_tag_t vlan_tag[N_TAGS];
header ipv4_t     ipv4;

parser start {
    return parse_ethernet;
}

parser parse_ethernet {
    extract(ethernet);
    return select(latest.ethertype) {
        0x8100 mask 0xEFFF: parse_vlan_tag;
        0x0800:             parse_ipv4;
        default: ingress;
    }
}

parser parse_vlan_tag {
    extract(vlan_tag[next]);
    return select(latest.ethertype) {
        0x8100 mask 0xEFFF: parse_vlan_tag;
        0x0800:             parse_ipv4;
        default: ingress;
    }
}

parser parse_ipv4 {
    extract(ipv4);
    return ingress;
}

action nop() {
}

table t1 {
    reads {
        ethernet.dstAddr : exact;
    }
    actions {
        nop;
    }
}

table t2 {
    reads {
        ethernet.srcAddr : exact;
    }
    actions {
        nop;
    }
}

control ingress {
    apply(t1);
}

control egress {
    apply(t2);
}
