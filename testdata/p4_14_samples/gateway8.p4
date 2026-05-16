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
action setb1(val, port) {
    modify_field(data.b1, val);
    modify_field(standard_metadata.egress_spec, port);
}


table test1 {
    reads {
        data.f1 : exact;
    }
    actions {
        setb1;
        noop;
    }
}
table test2 {
    reads {
        data.f2 : exact;
    }
    actions {
        setb1;
        noop;
    }
}

control ingress {
    if (data.b2 == 1 or data.b2 == 2 or data.b2 == 5 or data.b2 == 10 or
        data.b2 == 17 or data.b2 == 19)
    {
        apply(test1);
    } else {
        apply(test2);
    }
}
