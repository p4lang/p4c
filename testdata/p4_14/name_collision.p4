/*
 * Copyright 2018 Barefoot Networks, Inc.
 * SPDX-FileCopyrightText: 2018 Barefoot Networks, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

header_type A {
    fields {
        A : 32;
        B : 32;
        h1 : 16;
        b1 : 8;
        b2 : 8;
    }
}
header_type B {
    fields {
        A : 9;
        B : 10;
    }
}
header A A;
metadata B meta;

parser start { return A; }

parser A {
    extract(A);
    return ingress;
}

counter B {
    type: packets_and_bytes;
    instance_count: 1024;
}

action noop() { }
action A(val, port, idx) {
    modify_field(A.b1, val);
    modify_field(standard_metadata.egress_spec, port);
    modify_field(meta.B, idx);
}
action B() { count(B, meta.B); }

table A {
    reads {
        A.A : exact;
    }
    actions {
        A;
        noop;
    }
}

table B {
    actions { B; }
    default_action: B();
}

control ingress {
    apply(A);
    apply(B);
}
