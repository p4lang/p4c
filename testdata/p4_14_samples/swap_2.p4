/*
 * SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
 * Copyright 2013-present Barefoot Networks, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

header_type hdr2_t {
    fields {
        f1 : 8;
        f2 : 8;
        f3 : 16;
    }
}

header hdr2_t hdr2;

parser start {
    extract(hdr2);
    return ingress;
}

action a21() {
    modify_field(standard_metadata.egress_spec, 3);
}

action a22() {
    modify_field(standard_metadata.egress_spec, 4);
}

table t_ingress_2 {
    reads {
        hdr2.f1 : exact;
    }
    actions {
        a21; a22;
    }
    size : 64;
}

control ingress {
    apply(t_ingress_2);
}

control egress {
}
