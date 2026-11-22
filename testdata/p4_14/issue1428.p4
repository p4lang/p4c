/*
 * SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
 * Copyright 2013-present Barefoot Networks, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

header_type ipv4_t {
    fields {
        version        : 4;
        ihl            : 4;
        diffserv       : 8;
        totalLen       : 16;
        identification : 16;
        flags          : 3;
        fragOffset     : 13;
        ttl            : 8;
        protocol       : 8;
        hdrChecksum    : 16;
        srcAddr        : 32;
        dstAddr        : 32;
    }
}

header_type tcp_t {
    fields {
        srcPort     : 16;
        dstPort     : 16;
        seqNo       : 32;
        ackNo       : 32;
        dataOffset  : 4;
        res         : 4;
        flags       : 8;
        window      : 16;
    }
}

header ipv4_t          ipv4;
header tcp_t           tcp;

field_list tcp_checksum_list {
    ipv4.srcAddr;
    ipv4.dstAddr;
    8'0;
    ipv4.protocol;
    tcp.srcPort;
    tcp.dstPort;
    tcp.seqNo;
    tcp.ackNo;
    tcp.dataOffset;
    tcp.res;
    tcp.flags;
    tcp.window;
    payload;
}

parser start {
    return ingress;
}

control ingress {
}

control egress {
}