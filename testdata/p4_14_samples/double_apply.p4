/*
 * SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
 * Copyright 2013-present Barefoot Networks, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

header_type h {
    fields {
        b : 1;
    }
}

metadata h m;

parser start {
    return ingress;
}

action x() {}

table t {
    actions { x; }
}

control c {
    apply(t);
}

control e {
    apply(t);
}

control ingress {
    c();
}
