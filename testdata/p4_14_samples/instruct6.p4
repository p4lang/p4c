/*
 * SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
 * Copyright 2013-present Barefoot Networks, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

header_type data_t {
    fields {
        f1 : 32;
        f2 : 32;
        h1 : 16;
        h2 : 16;
        b1 : 8;
        b2 : 8;
    }
}
header data_t data;

parser start {
    extract(data);
    return ingress;
}

action noop() { }
action op1(port) {
    modify_field_with_shift(data.h1, data.h2, 8, 0xff);
    modify_field(standard_metadata.egress_spec, port);
}
action op2(port) {
    modify_field_with_shift(data.h1, data.h2, 4, 0xf0);
    modify_field(standard_metadata.egress_spec, port);
}

table test1 {
    reads { data.f1 : exact; }
    actions { op1; op2; noop; }
}

control ingress {
    apply(test1);
}
