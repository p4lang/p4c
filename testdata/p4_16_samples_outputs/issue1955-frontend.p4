#include <core.p4>
#include <v1model.p4>

typedef bit<48> EthernetAddress;
header ethernet_t {
    EthernetAddress dstAddr;
    EthernetAddress srcAddr;
    bit<16>         etherType;
}

header ipv4_t {
    bit<4>  version;
    bit<4>  ihl;
    bit<8>  diffserv;
    bit<16> totalLen;
    bit<16> identification;
    bit<3>  flags;
    bit<13> fragOffset;
    bit<8>  ttl;
    bit<8>  protocol;
    bit<16> hdrChecksum;
    bit<32> srcAddr;
    bit<32> dstAddr;
}

struct headers_t {
    ethernet_t ethernet_1;
    ipv4_t     ipv4_1;
    ethernet_t ethernet_2;
    ipv4_t     ipv4_2;
}

struct metadata_t {
}

extern E {
    E();
    void g();
}

parser parserImpl(packet_in packet, out headers_t hdr, inout metadata_t meta, inout standard_metadata_t stdmeta) {
    @name("parserImpl.p1.ipv4_ethertypes") value_set<bit<16>>(8) p1_ipv4_ethertypes;
    @name("parserImpl.p2.ipv4_ethertypes") value_set<bit<16>>(8) p2_ipv4_ethertypes;
    state start {
        transition subParser_start;
    }
    state subParser_start {
        packet.extract<ethernet_t>(hdr.ethernet_1);
        transition select(hdr.ethernet_1.etherType) {
            p1_ipv4_ethertypes: subParser_parse_ipv4;
            default: start_0;
        }
    }
    state subParser_parse_ipv4 {
        packet.extract<ipv4_t>(hdr.ipv4_1);
        transition start_0;
    }
    state start_0 {
        transition subParser_start_0;
    }
    state subParser_start_0 {
        packet.extract<ethernet_t>(hdr.ethernet_2);
        transition select(hdr.ethernet_2.etherType) {
            p2_ipv4_ethertypes: subParser_parse_ipv4_0;
            default: start_1;
        }
    }
    state subParser_parse_ipv4_0 {
        packet.extract<ipv4_t>(hdr.ipv4_2);
        transition start_1;
    }
    state start_1 {
        transition accept;
    }
}

control verifyChecksum(inout headers_t hdr, inout metadata_t meta) {
    apply {
    }
}

control ingressImpl(inout headers_t hdr, inout metadata_t meta, inout standard_metadata_t stdmeta) {
    apply {
    }
}

control egressImpl(inout headers_t hdr, inout metadata_t meta, inout standard_metadata_t stdmeta) {
    apply {
    }
}

control updateChecksum(inout headers_t hdr, inout metadata_t meta) {
    apply {
    }
}

control deparserImpl(packet_out packet, in headers_t hdr) {
    apply {
        packet.emit<ethernet_t>(hdr.ethernet_1);
        packet.emit<ipv4_t>(hdr.ipv4_1);
        packet.emit<ethernet_t>(hdr.ethernet_2);
        packet.emit<ipv4_t>(hdr.ipv4_2);
    }
}

V1Switch<headers_t, metadata_t>(parserImpl(), verifyChecksum(), ingressImpl(), egressImpl(), updateChecksum(), deparserImpl()) main;

