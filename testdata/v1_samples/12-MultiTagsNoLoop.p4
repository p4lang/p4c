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

header_type my_tag_t {
    fields {
        bd : 32;
        ethertype : 16;
    }
}

#define N_TAGS 1

#if N_TAGS > 1 
#  define ARRAY_DECL [N_TAGS]
#  define ARRAY_REF  [next]
#else 
#  define ARRAY_DECL
#  define ARRAY_REF 
#endif

header ethernet_t ethernet;
header vlan_tag_t vlan_tag ARRAY_DECL;
header my_tag_t   my_tag   ARRAY_DECL;

parser start {
    extract(ethernet);
    return select(latest.ethertype) {
        0x8100 mask 0xEFFF: parse_vlan_tag;
        0x9000            : parse_my_tag;
        default: ingress;
    }
}

parser parse_vlan_tag {
    extract(vlan_tag ARRAY_REF);
    return select(latest.ethertype) {
        0x9000            : parse_my_tag;
        default: ingress;
    }
}

parser parse_my_tag {
    extract(my_tag ARRAY_REF);
    return select(latest.ethertype) {
        0x8100 mask 0xEFFF: parse_vlan_tag;
        default: ingress;
    }
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
