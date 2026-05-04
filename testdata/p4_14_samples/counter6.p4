/*
 * SPDX-FileCopyrightText: 2019 Barefoot Networks, Inc.
 * Copyright 2019-present Barefoot Networks, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
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
