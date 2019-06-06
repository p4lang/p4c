/*
 * Copyright 2017-present Open Networking Foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// All of the P4 source files in this directory were copied from the
// following location on 2019-Apr-20:

// https://github.com/opennetworkinglab/onos/blob/master/pipelines/fabric/src/main/resources

// The only changes made were:

// + Replace all calls to mark_to_drop() with calls to
//   mark_to_drop(standard_metadata), so that they would compile with
//   the latest version of p4c as of 2019-Apr-20.

// + Add the following few #define lines, which in the original code
//   were specified via command line arguments in a shell script
//   bmv2-compile.sh

#define TARGET_BMV2
#define CPU_PORT           255
#define WITH_PORT_COUNTER
#define WITH_SPGW

#include <core.p4>
#include <v1model.p4>

#include "include/size.p4"
#include "include/control/filtering.p4"
#include "include/control/forwarding.p4"
#include "include/control/acl.p4"
#include "include/control/next.p4"
#include "include/control/packetio.p4"
#include "include/header.p4"
#include "include/checksum.p4"
#include "include/parser.p4"

#ifdef WITH_PORT_COUNTER
#include "include/control/port_counter.p4"
#endif // WITH_PORT_COUNTER

#ifdef WITH_SPGW
#include "include/spgw.p4"
#endif // WITH_SPGW

#ifdef WITH_INT
#include "include/int/int_main.p4"
#endif // WITH_INT

control FabricIngress (inout parsed_headers_t hdr,
                       inout fabric_metadata_t fabric_metadata,
                       inout standard_metadata_t standard_metadata) {

    PacketIoIngress() pkt_io_ingress;
    Filtering() filtering;
    Forwarding() forwarding;
    Acl() acl;
    Next() next;
#ifdef WITH_PORT_COUNTER
    PortCountersControl() port_counters_control;
#endif // WITH_PORT_COUNTER

    apply {
        _PRE_INGRESS
#ifdef WITH_SPGW
        spgw_normalizer.apply(hdr.gtpu.isValid(), hdr.gtpu_ipv4, hdr.gtpu_udp,
                              hdr.ipv4, hdr.udp, hdr.inner_ipv4, hdr.inner_udp);
#endif // WITH_SPGW
        pkt_io_ingress.apply(hdr, fabric_metadata, standard_metadata);
        filtering.apply(hdr, fabric_metadata, standard_metadata);
#ifdef WITH_SPGW
        spgw_ingress.apply(hdr.gtpu_ipv4, hdr.gtpu_udp, hdr.gtpu,
                           hdr.ipv4, hdr.udp, fabric_metadata,
                           standard_metadata);
#endif // WITH_SPGW
        if (fabric_metadata.skip_forwarding == _FALSE) {
            forwarding.apply(hdr, fabric_metadata, standard_metadata);
        }
        acl.apply(hdr, fabric_metadata, standard_metadata);
        if (fabric_metadata.skip_next == _FALSE) {
            next.apply(hdr, fabric_metadata, standard_metadata);
#ifdef WITH_PORT_COUNTER
            // FIXME: we're not counting pkts punted to cpu or forwarded via
            // multicast groups. Remove when gNMI support will be there.
            port_counters_control.apply(hdr, fabric_metadata, standard_metadata);
#endif // WITH_PORT_COUNTER
#if defined(WITH_INT_SOURCE) || defined(WITH_INT_SINK)
            process_set_source_sink.apply(hdr, fabric_metadata, standard_metadata);
#endif
        }
    }
}

control FabricEgress (inout parsed_headers_t hdr,
                      inout fabric_metadata_t fabric_metadata,
                      inout standard_metadata_t standard_metadata) {

    PacketIoEgress() pkt_io_egress;
    EgressNextControl() egress_next;

    apply {
        _PRE_EGRESS
        pkt_io_egress.apply(hdr, fabric_metadata, standard_metadata);
        egress_next.apply(hdr, fabric_metadata, standard_metadata);
#ifdef WITH_SPGW
        spgw_egress.apply(hdr.ipv4, hdr.gtpu_ipv4, hdr.gtpu_udp, hdr.gtpu,
                          fabric_metadata, standard_metadata);
#endif // WITH_SPGW
#ifdef WITH_INT
        process_int_main.apply(hdr, fabric_metadata, standard_metadata);
#endif
    }
}

V1Switch(
    FabricParser(),
    FabricVerifyChecksum(),
    FabricIngress(),
    FabricEgress(),
    FabricComputeChecksum(),
    FabricDeparser()
) main;
