/*
 * SPDX-FileCopyrightText: 2017 Open Networking Foundation
 * Copyright 2017-present Open Networking Foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __PORT_COUNTER__
#define __PORT_COUNTER__
#include "../define.p4"
#include "../header.p4"

control PortCountersControl(inout parsed_headers_t hdr,
                            inout fabric_metadata_t fabric_metadata,
                            inout standard_metadata_t standard_metadata) {

    counter(MAX_PORTS, CounterType.packets_and_bytes) egress_port_counter;
    counter(MAX_PORTS, CounterType.packets_and_bytes) ingress_port_counter;

    apply {
        if (standard_metadata.egress_spec < MAX_PORTS) {
            egress_port_counter.count((bit<32>)standard_metadata.egress_spec);
        }
        if (standard_metadata.ingress_port < MAX_PORTS) {
            ingress_port_counter.count((bit<32>)standard_metadata.ingress_port);
        }
    }
}
#endif
