/*
 * Copyright 2017 VMware, Inc.
 * SPDX-FileCopyrightText: 2017 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <core.p4>
#include <v1model.p4>

typedef standard_metadata_t std_meta_t;

struct H { }

struct M { bool flag; }

control DeparserI(packet_out packet, in H hdr) {
    apply { }
}

parser ParserI(packet_in pk, out H hdr, inout M meta, inout std_meta_t std_meta) {
    state start { transition accept; }
}

control VerifyChecksumI(inout H hdr, inout M meta) {
    apply { }
}

control ComputeChecksumI(inout H hdr, inout M meta) {
    apply { }
}

control IngressI(inout H hdr, inout M meta, inout std_meta_t std_meta) {
    apply {
        meta.flag = true;
    }
}

control EgressI(inout H hdr, inout M meta, inout std_meta_t std_meta) {
    apply { }
}

V1Switch(ParserI(), VerifyChecksumI(), IngressI(), EgressI(), ComputeChecksumI(), DeparserI()) main;
