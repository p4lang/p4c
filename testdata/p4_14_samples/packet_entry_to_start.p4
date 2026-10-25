/*
 * SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
 * Copyright 2013-present Barefoot Networks, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

@pragma packet_entry
parser start_i2e_mirrored {
    return start;
}

parser start {
    return ingress;
}

header_type start {
    fields {
        f1 : 32;
    }
}

metadata start m;

action a1() {
    modify_field(m.f1, 1);
}

table t1 {
    actions {
        a1;
    }
}

control ingress {
    apply(t1);
}

control egress {
}