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
    srcRoute_t              srcRoutes1;
    srcRoute_t              srcRoutes2;
    srcRoute_t              srcRoutes3;
    ipv4_t                  ipv4;
    bit<32>                 index;
}

parser MyParser(packet_in packet,
                out headers hdr,
                inout metadata meta,
                inout standard_metadata_t standard_metadata) {
    state start {
        hdr.index = 0;
        packet.extract(hdr.ethernet);
        transition select(hdr.ethernet.etherType) {
            0: last;
            1: access1;
            2: access2;
            3: access3;
            default: accept;
        }
    }

    state last {
        hdr.index = hdr.index + 1;
        transition accept;
    }

    state access1 {
        hdr.index = hdr.index + 1;
        packet.extract(hdr.srcRoutes1);
        transition last;
    }

    state access2 {
        hdr.index = hdr.index + 1;
        packet.extract(hdr.srcRoutes2);
        transition access1;
    }

    state access3 {
        hdr.index = hdr.index + 1;
        packet.extract(hdr.srcRoutes3);
        transition access2;
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