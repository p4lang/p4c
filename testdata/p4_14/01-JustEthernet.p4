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

parser start {
    extract(ethernet);
    return ingress;
}

action nop() {
}

table t1 {
    reads {
        ethernet.dstAddr : exact;
    }
    actions {
        nop;
    }
}

table t2 {
    reads {
        ethernet.srcAddr : exact;
    }
    actions {
        nop;
    }
}

control ingress {
//    apply(t1);
}

control egress {
//    apply(t2);
}
