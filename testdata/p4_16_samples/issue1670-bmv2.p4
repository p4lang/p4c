/*******************************************************************************
 * BAREFOOT NETWORKS CONFIDENTIAL & PROPRIETARY
 *
 * Copyright (c) 2018-2019 Barefoot Networks, Inc.
 * All Rights Reserved.
 *
 * NOTICE: All information contained herein is, and remains the property of
 * Barefoot Networks, Inc. and its suppliers, if any. The intellectual and
 * technical concepts contained herein are proprietary to Barefoot Networks,
 * Inc.
 * and its suppliers and may be covered by U.S. and Foreign Patents, patents in
 * process, and are protected by trade secret or copyright law.
 * Dissemination of this information or reproduction of this material is
 * strictly forbidden unless prior written permission is obtained from
 * Barefoot Networks, Inc.
 *
 * No warranty, explicit or implicit is provided, unless granted under a
 * written agreement with Barefoot Networks, Inc.
 *
 ******************************************************************************/

#include <core.p4>
#include <v1model.p4>

struct switch_metadata_t {
    bit<9> port;
}

header serialized_switch_metadata_t {
    switch_metadata_t meta;
}

struct parsed_packet_t {
    serialized_switch_metadata_t mirrored_md;
}

struct local_metadata_t {
}

parser parse(packet_in pk, out parsed_packet_t h, inout local_metadata_t local_metadata,
             inout standard_metadata_t standard_metadata) {
    state start {
	transition accept;
    }
}

control ingress(inout parsed_packet_t h, inout local_metadata_t local_metadata,
                inout standard_metadata_t standard_metadata) {
    apply {
	h.mirrored_md.setValid();
	h.mirrored_md.meta = { 9w0 };
    }
}

control egress(inout parsed_packet_t hdr, inout local_metadata_t local_metadata,
               inout standard_metadata_t standard_metadata) {
    apply { }
}

control deparser(packet_out b, in parsed_packet_t h) {
    apply { }
}

control verify_checksum(inout parsed_packet_t hdr,
inout local_metadata_t local_metadata) {
    apply { }
}

control compute_checksum(inout parsed_packet_t hdr,
                         inout local_metadata_t local_metadata) {
    apply { }
}

V1Switch(parse(), verify_checksum(), ingress(), egress(),
compute_checksum(), deparser()) main;
