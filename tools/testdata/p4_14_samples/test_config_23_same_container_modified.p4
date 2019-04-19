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

/* Sample P4 program */
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

header_type vlan_t {
     fields {
         pcp : 3;
         cfi : 1;
         vid : 12;
         etherType : 16;
     }
}

parser start {
    return parse_ethernet;
}

header ethernet_t ethernet;

parser parse_ethernet {
    extract(ethernet);
    return select(ethernet.etherType) {
        0x800 : parse_ipv4;
        0x8100: parse_vlan;
        default: ingress;
    }
}

header ipv4_t ipv4;
header vlan_t vlan;

parser parse_ipv4 {
    extract(ipv4);
    return ingress;
}

parser parse_vlan {
    extract(vlan);
    return ingress;
}


action action_0(my_param0, my_param1){
    modify_field(ipv4.protocol, my_param0, 0xF8);
    modify_field(ipv4.ttl, my_param1);
    //modify_field(ipv4.totalLen, ipv4.hdrChecksum);
    //modify_field(ipv4.protocol, ipv4.ttl);
}

action action_1(my_param2) {
    modify_field(ipv4.totalLen, ipv4.totalLen);
}


table table_0 {
   reads {
     ipv4.srcAddr : exact;
   }
   actions {
     action_0;
     action_1;
   }
   max_size : 4096;
}


/* Main control flow */

control ingress {
    apply(table_0);
}
