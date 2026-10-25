/*
 * SPDX-FileCopyrightText: 2017 Open Networking Foundation
 * Copyright 2017-present Open Networking Foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __CHECKSUM__
#define __CHECKSUM__

#ifdef WITH_SPGW
#include "spgw.p4"
#endif // WITH_SPGW

control FabricComputeChecksum(inout parsed_headers_t hdr,
                              inout fabric_metadata_t meta)
{
    apply {
        update_checksum(hdr.ipv4.isValid(),
            {
                hdr.ipv4.version,
                hdr.ipv4.ihl,
                hdr.ipv4.dscp,
                hdr.ipv4.ecn,
                hdr.ipv4.total_len,
                hdr.ipv4.identification,
                hdr.ipv4.flags,
                hdr.ipv4.frag_offset,
                hdr.ipv4.ttl,
                hdr.ipv4.protocol,
                hdr.ipv4.src_addr,
                hdr.ipv4.dst_addr
            },
            hdr.ipv4.hdr_checksum,
            HashAlgorithm.csum16
        );
#ifdef WITH_SPGW
        update_gtpu_checksum.apply(hdr.gtpu_ipv4, hdr.gtpu_udp, hdr.gtpu,
                                   hdr.ipv4, hdr.udp);
#endif // WITH_SPGW
    }
}

control FabricVerifyChecksum(inout parsed_headers_t hdr,
                             inout fabric_metadata_t meta)
{
    apply {
        verify_checksum(hdr.ipv4.isValid(),
            {
                hdr.ipv4.version,
                hdr.ipv4.ihl,
                hdr.ipv4.dscp,
                hdr.ipv4.ecn,
                hdr.ipv4.total_len,
                hdr.ipv4.identification,
                hdr.ipv4.flags,
                hdr.ipv4.frag_offset,
                hdr.ipv4.ttl,
                hdr.ipv4.protocol,
                hdr.ipv4.src_addr,
                hdr.ipv4.dst_addr
            },
            hdr.ipv4.hdr_checksum,
            HashAlgorithm.csum16
        );
    }
}

#endif
