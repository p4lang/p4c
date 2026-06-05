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
        b3 : 8;
        b4 : 8;
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
    if (1 == (15 & data.b2)) {
        apply(test1);
    } else {
        apply(test2);
    }
}
