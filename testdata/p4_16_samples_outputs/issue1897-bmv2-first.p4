#include <core.p4>
#include <v1model.p4>

header addr_type_t {
    bit<8> dstType;
    bit<8> srcType;
}

header addr_ipv4_t {
    bit<32> addr;
}

header addr_ipv6_t {
    bit<128> addr;
}

header_union addr_t {
    addr_ipv4_t ipv4;
    addr_ipv6_t ipv6;
}

struct metadata {
}

struct headers {
    addr_type_t addr_type;
    addr_t      addr_dst;
    addr_t      addr_src;
}

parser ProtAddrParser(packet_in packet, in bit<8> addrType, out addr_t addr) {
    state start {
        transition select(addrType) {
            8w0x1: ipv4;
            8w0x2: ipv6;
        }
    }
    state ipv4 {
        packet.extract<addr_ipv4_t>(addr.ipv4);
        transition accept;
    }
    state ipv6 {
        packet.extract<addr_ipv6_t>(addr.ipv6);
        transition accept;
    }
}

parser ProtParser(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    ProtAddrParser() addrParser;
    state start {
        packet.extract<addr_type_t>(hdr.addr_type);
        addrParser.apply(packet, hdr.addr_type.dstType, hdr.addr_dst);
        addrParser.apply(packet, hdr.addr_type.srcType, hdr.addr_src);
        transition accept;
    }
}

control ProtVerifyChecksum(inout headers hdr, inout metadata meta) {
    apply {
    }
}

control ProtIngress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control ProtEgress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control ProtComputeChecksum(inout headers hdr, inout metadata meta) {
    apply {
    }
}

control ProtDeparser(packet_out packet, in headers hdr) {
    apply {
        packet.emit<headers>(hdr);
    }
}

V1Switch<headers, metadata>(ProtParser(), ProtVerifyChecksum(), ProtIngress(), ProtEgress(), ProtComputeChecksum(), ProtDeparser()) main;

