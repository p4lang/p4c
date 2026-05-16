/*
 * Copyright 2016 VMware, Inc.
 * SPDX-FileCopyrightText: 2016 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <core.p4>
#include <v1model.p4>

struct H { bit<32> x; };
struct M { };

parser ParserI(packet_in b, out H parsedHdr, inout M meta,
              inout standard_metadata_t standard_metadata) {
    state start {
        transition accept;
    }
}


control VerifyChecksumI(in H hdr,
                        inout M meta) {
    apply { }
}


control IngressI(inout H hdr,
                 inout M meta,
                 inout standard_metadata_t standard_metadata) {
    apply { }
}


control EgressI(inout H hdr,
                inout M meta,
                inout standard_metadata_t standard_metadata) {
    apply { }
}


control ComputeChecksumI(inout H hdr,
                         inout M meta) {
    apply { }
}


control DeparserI(packet_out b, in H hdr) {
    apply {
        b.emit(hdr);  // illegal data type
    }
}


V1Switch(ParserI(),
         VerifyChecksumI(),
         IngressI(),
         EgressI(),
         ComputeChecksumI(),
         DeparserI()) main;
