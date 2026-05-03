/*
 * SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
 * Copyright 2013-present Barefoot Networks, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

header_type ethernet_t {
    fields {
        dstAddr   : 48;
        srcAddr   : 48;
        ethertype : 16;
    }
}

header ethernet_t ethernet;

header_type h_t {
    fields {
        f1 : 16;
    }
}

header h_t h;

parser start {
    extract(ethernet);
    return ingress;
}

action nop() {
}

table t1 {
    actions {
        nop;
    }
}

control ingress {
    if (h.f1 > 1) {
        apply(t1);
    }
}

control egress {
}
