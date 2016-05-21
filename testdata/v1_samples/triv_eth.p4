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

// Standard L2 Ethernet header
header_type ethernet_t {
    fields {
        dst_addr        : 48; // width in bits
        src_addr        : 48;
        ethertype       : 16;
    }
}

header ethernet_t ethernet;

// just ethernet
parser start {
    extract(ethernet);
    return ingress;
}

action route_eth(egress_spec, src_addr) {
    modify_field(standard_metadata.egress_spec, egress_spec);
    modify_field(ethernet.src_addr, src_addr);
}
action noop() { }

table routing {
    reads {
	ethernet.dst_addr : lpm;
    }
    actions {
	route_eth;
	noop;
    }
}

control ingress {
    apply(routing);
}
