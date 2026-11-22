/*
 * Copyright 2017 VMware, Inc.
 * SPDX-FileCopyrightText: 2017 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
header_type ethernet_t {
    fields {
        dstAddr : 48;
        srcAddr : 48;
        ethertype : 16;
    }
}

header ethernet_t ethernet;

parser start {
    extract(ethernet);
    return ingress;
}

action set_egress_port(port) {
    modify_field(standard_metadata.egress_spec, port);
}

table t1 {
    reads {
         ethernet.dstAddr : lpm;
         ethernet.srcAddr : lpm;
    }
    actions {
         set_egress_port;
    }
}

control ingress {
    apply(t1);
}

control egress {
}