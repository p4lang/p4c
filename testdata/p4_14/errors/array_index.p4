/*
 * SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
 * Copyright 2013-present Barefoot Networks, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

header_type data_t {
    fields {
        f1 : 32;
    }
}

header data_t   data[4];

parser start {
    extract(data[next]);
    return ingress;
}

action a(port, val) {
    modify_field(data[port].f1, val);
}

table test1 {
    actions {
        a;
        noop;
    }
}

control ingress {
    apply(test1);
}
