#include "v1model.p4"

const bit<16> TYPE_IPV4 = 0x800;
const bit<16> TYPE_SRCROUTING = 0x1234;
const bit<16> MAX_HOPS = 4;
/*
+-----------------------------+
| ethernet !(0x0800 or 0x1234)|
+-----------------------------+
ors
+-----------------+------------+------------+---+------------+------+
| ethernet 0x1234 | srcRoute 0 | srcRoute 0 |...| srcRoute 1 | ipv4 |
+-----------------+------------+------------+---+------------+------+
*/

typedef bit<9>  egressSpec_t;
typedef bit<48> macAddr_t;
typedef bit<32> ip4Addr_t;

header ethernet_t {
    macAddr_t dstAddr;
    macAddr_t srcAddr;
    bit<16>   etherType;
}

header srcRoute_t {
    bit<2>    bos;   /* Bottom of stack */
    bit<15>   port;
}

header ipv4_t {
    bit<4>    version;
    bit<4>    ihl;
    bit<8>    diffserv;
    bit<16>   totalLen;
    bit<16>   identification;
    bit<3>    flags;
    bit<13>   fragOffset;
    bit<8>    ttl;
    bit<8>    protocol;
    bit<16>   hdrChecksum;
    ip4Addr_t srcAddr;
    ip4Addr_t dstAddr;
}

struct metadata {
    /* empty */
}

struct headers {
    ethernet_t              ethernet;
    srcRoute_t[MAX_HOPS]    srcRoutes;
    ipv4_t                  ipv4;
    bit<32>                 index;
}

parser MyParser(packet_in packet,
                out headers hdr,
                inout metadata meta,
                inout standard_metadata_t standard_metadata) {
    state start {
        hdr.index = 0;
        transition parse_ethernet;
    }

    state parse_ethernet {
        packet.extract(hdr.ethernet);
        transition select(hdr.ethernet.etherType) {
            TYPE_SRCROUTING: parse_srcRouting;
            default: accept;
        }
    }

    state parse_srcRouting {
        packet.extract(hdr.srcRoutes.next); 
        transition select(hdr.srcRoutes.last.bos) {
            1: parse_ipv4;
            2: callMidle;
            default: 
                callMidle;
        }
    }

    state callLast {
        packet.extract(hdr.srcRoutes.next);
        transition select(hdr.srcRoutes.last.bos) {
            1: parse_ipv4;
            default:
                parse_srcRouting;
        }
    }

    state callMidle {
        hdr.index = hdr.index + 1;
        transition callLast;
    }

    state parse_ipv4 {
        packet.extract(hdr.ipv4);
        transition accept;
    }
}

control mau(inout headers hdr, inout metadata meta, inout standard_metadata_t sm) {
apply {}
}
control deparse(packet_out pkt, in headers hdr) {
apply {}
}
control verifyChecksum(inout headers hdr, inout metadata meta) {
apply {}
}
control computeChecksum(inout headers hdr, inout metadata meta) {
apply {}
}
V1Switch(MyParser(), verifyChecksum(), mau(), mau(), computeChecksum(),
        deparse()) main;