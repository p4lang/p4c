/*
Copyright 2019-present Barefoot Networks, Inc.

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

/*
 * Simple program, just a direct mapped RAM
 */

header_type type_t { fields { type : 16; } }
header_type A_t {
    fields {
        f1 : 32;
        f2 : 32;
        h1 : 16;
        b1 : 8;
        b2 : 8;
} }
header_type B_t {
    fields {
        b1 : 8;
        b2 : 8;
        h1 : 16;
        f1 : 32;
        f2 : 32;
} }
header type_t packet_type;
header A_t hdrA;
header B_t hdrB;

parser start {
    extract(packet_type);
    return select(packet_type.type) {
    0xA mask 0xf : parseA;
    0xB mask 0xf : parseB;
    }
}
parser parseA {
    extract(hdrA);
    return ingress;
}
parser parseB {
    extract(hdrB);
    return ingress;
}

counter cntDum {
	type: packets;
        instance_count: 4096;
        min_width:64;
}
action act(port, idx) {
    modify_field(standard_metadata.egress_spec, port);
    count(cntDum, idx);
}

table tabA {
    reads {
        hdrA.f1 : exact;
    }
    actions {
        act;
    }
}
control processA { apply(tabA); }

table tabB {
    reads {
        hdrB.f1 : exact;
    }
    actions {
        act;
    }
}
control processB { apply(tabB); }

control ingress {
    if (valid(hdrA)) {
        processA();
    } else if (valid(hdrB)) {
        processB();
    }
}
control egress {
}
