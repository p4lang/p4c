/*
 * SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
 * Copyright 2013-present Barefoot Networks, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

parser start {
    return ingress;
}

action a1() {
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
