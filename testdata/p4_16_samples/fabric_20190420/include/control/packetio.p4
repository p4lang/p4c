/*
 * SPDX-FileCopyrightText: 2017 Open Networking Foundation
 * Copyright 2017-present Open Networking Foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "../header.p4"

control PacketIoIngress(inout parsed_headers_t hdr,
                        inout fabric_metadata_t fabric_metadata,
                        inout standard_metadata_t standard_metadata) {

    apply {
        if (hdr.packet_out.isValid()) {
            standard_metadata.egress_spec = hdr.packet_out.egress_port;
            hdr.packet_out.setInvalid();
            fabric_metadata.is_controller_packet_out = _TRUE;
            // No need for ingress processing, straight to egress.
            exit;
        }
    }
}

control PacketIoEgress(inout parsed_headers_t hdr,
                       inout fabric_metadata_t fabric_metadata,
                       inout standard_metadata_t standard_metadata) {

    apply {
        if (fabric_metadata.is_controller_packet_out == _TRUE) {
            // Transmit right away.
            exit;
        }
        if (standard_metadata.egress_port == CPU_PORT) {
            if (fabric_metadata.is_multicast == _TRUE &&
                fabric_metadata.clone_to_cpu == _FALSE) {
                // Is multicast but clone was not requested.
                mark_to_drop(standard_metadata);
            }
            hdr.packet_in.setValid();
            hdr.packet_in.ingress_port = standard_metadata.ingress_port;
            // No need to process through the rest of the pipeline.
            exit;
        }
    }
}
