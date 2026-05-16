/*
 * SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
 * Copyright 2013-present Barefoot Networks, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

header_type hdrA_t {
    fields {
        f1 : 8;
        f2 : 64;
    }
}

header hdrA_t hdrA;

parser start {
    extract(hdrA);
    return ingress;
}

action _nop() {

}

action _truncate(new_length, port) {
    modify_field(standard_metadata.egress_spec, port);
    truncate(new_length);
}

table t_ingress {
    reads {
        hdrA.f1 : exact;
    }
    actions {
        _nop; _truncate;
    }
    size : 128;
}

control ingress {
    apply(t_ingress);
}

control egress {
}
