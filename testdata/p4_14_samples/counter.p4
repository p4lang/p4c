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

header_type meta_t {
    fields {
        register_tmp : 32;
    }
}

metadata meta_t meta;

parser start {
    return parse_ethernet;
}

header ethernet_t ethernet;
metadata intrinsic_metadata_t intrinsic_metadata;

parser parse_ethernet {
    extract(ethernet);
    return ingress;
}

action _drop() {
    drop();
}

action _nop() {
}

counter my_indirect_counter {
    type: packets;
    static: m_table;
    instance_count: 16384;
}

counter my_direct_counter {
    type: bytes;
    direct: m_table;
}

action m_action(idx) {
    count(my_indirect_counter, idx);
    drop();
}

table m_table {
    reads {
        ethernet.srcAddr : exact;
    }
    actions {
        m_action; _nop;
    }
    size : 16384;
}

control ingress {
    apply(m_table);
}

control egress {
}
