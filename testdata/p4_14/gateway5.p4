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
        x1 : 2;
        pad0 : 3;
        x2 : 2;
        pad1 : 5;
        x3 : 1;
        pad2 : 3;
        skip : 32;
        x4 : 1;
        x5 : 1;
        pad3 : 6;
    }
}
header data_t data;

parser start {
    extract(data);
    return ingress;
}

action noop() { }
action output(port) { modify_field(standard_metadata.egress_spec, port); }

table test1 {
    reads {
        data.f1 : exact;
    }
    actions {
        output;
        noop;
    }
}
table test2 {
    reads {
        data.f2 : exact;
    }
    actions {
        output;
        noop;
    }
}

control ingress {
    if (data.x2 == 1 and data.x4 == 0) {
        apply(test1);
    } else {
        apply(test2);
    }
}
