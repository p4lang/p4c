/*
 * Copyright 2016 VMware, Inc.
 * SPDX-FileCopyrightText: 2016 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <core.p4>
#include <v1model.p4>

struct H { };
struct M { };

parser ParserI(packet_in b, out H parsedHdr, inout M meta,
              inout standard_metadata_t standard_metadata) {
    state start {
        b.extract(parsedHdr);  // illegal data type
        transition accept;
    }
}


control VerifyChecksumI(in H hdr,
                        inout M meta,
                        inout standard_metadata_t standard_metadata) {
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
                         inout M meta,
                         inout standard_metadata_t standard_metadata) {
    apply { }
}


control DeparserI(packet_out b, in H hdr) {
    apply { }
}


V1Switch(ParserI(),
         VerifyChecksumI(),
         IngressI(),
         EgressI(),
         ComputeChecksumI(),
         DeparserI()) main;
