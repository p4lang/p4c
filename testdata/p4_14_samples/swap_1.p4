/*
 * SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
 * Copyright 2013-present Barefoot Networks, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

header_type hdr1_t {
    fields {
        f1 : 8;
        f2 : 8;
    }
}

header hdr1_t hdr1;

parser start {
    extract(hdr1);
    return ingress;
}

action a11() {
    modify_field(standard_metadata.egress_spec, 1);
}

action a12() {
    modify_field(standard_metadata.egress_spec, 2);
}

table t_ingress_1 {
    reads {
        hdr1.f1 : exact;
    }
    actions {
        a11; a12;
    }
    size : 128;
}

control ingress {
    apply(t_ingress_1);
}

control egress {
}
