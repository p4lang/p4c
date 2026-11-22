/*
 * SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
 * Copyright 2013-present Barefoot Networks, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Simple stat program.
 * Direct mapped, that does not go across stages
 */

header_type ethernet_t {
    fields {
        dstAddr : 48;
    }
}
parser start {
    return parse_ethernet;
}

header ethernet_t ethernet;

parser parse_ethernet {
    extract(ethernet);
    return ingress;
}
action act(port) {
    modify_field(standard_metadata.egress_spec, port);
}
table tab1 {
    reads {
        ethernet.dstAddr : ternary;
    }
    actions {
        act;
    }
  size: 6100;
}

counter cnt {
	type: packets;
	direct: tab1;

}
control ingress {
    apply(tab1);
}
control egress {

}
