/*
 * Copyright 2016 VMware, Inc.
 * SPDX-FileCopyrightText: 2016 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <v1model.p4>

struct h {
  bit<8> n;
}

struct m {
}

parser MyParser(packet_in b,
                out h parsedHdr,
                inout m meta,
                inout standard_metadata_t standard_metadata) {
  state start {
    transition accept;
  }
}

control MyVerifyChecksum(in h hdr,
                       inout m meta,
                       inout standard_metadata_t standard_metadata) {
  apply {}

}
control MyIngress(inout h hdr,
                  inout m meta,
                  inout standard_metadata_t standard_metadata) {
  apply {}
}
control MyEgress(inout h hdr,
               inout m meta,
               inout standard_metadata_t standard_metadata) {
  apply {}
}

control MyComputeChecksum(inout h hdr,
                          inout m meta,
                          inout standard_metadata_t standard_metadata) {
  apply {}
}
control MyDeparser(packet_out b, in h hdr) {
  apply {}
}

V1Switch(MyParser(),
         MyVerifyChecksum(),
         MyIngress(),
         MyEgress(),
         MyComputeChecksum(),
         MyDeparser()) main;