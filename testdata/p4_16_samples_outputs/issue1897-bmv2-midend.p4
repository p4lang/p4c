#include <core.p4>
#define V1MODEL_VERSION 20180101
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

struct metadata {
}

struct headers {
    addr_type_t addr_type;
    addr_ipv4_t addr_dst_ipv4;
    addr_ipv6_t addr_dst_ipv6;
    addr_ipv4_t addr_src_ipv4;
    addr_ipv6_t addr_src_ipv6;
}

parser ProtParser(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name("addr_0_ipv4") addr_ipv4_t addr_0_ipv4_0;
    @name("addr_0_ipv6") addr_ipv6_t addr_0_ipv6_0;
    state start {
        packet.extract<addr_type_t>(hdr.addr_type);
        addr_0_ipv4_0.setInvalid();
        addr_0_ipv6_0.setInvalid();
        transition select(hdr.addr_type.dstType) {
            8w0x1: ProtAddrParser_ipv4;
            8w0x2: ProtAddrParser_ipv6;
            default: noMatch;
        }
    }
    state ProtAddrParser_ipv4 {
        packet.extract<addr_ipv4_t>(addr_0_ipv4_0);
        transition start_0;
    }
    state ProtAddrParser_ipv6 {
        packet.extract<addr_ipv6_t>(addr_0_ipv6_0);
        transition start_0;
    }
    state start_0 {
        transition select(addr_0_ipv4_0.isValid()) {
            true: start_0_true;
            false: start_0_false;
        }
    }
    state start_0_true {
        hdr.addr_dst_ipv4.setValid();
        hdr.addr_dst_ipv4 = addr_0_ipv4_0;
        hdr.addr_dst_ipv6.setInvalid();
        transition start_0_join;
    }
    state start_0_false {
        hdr.addr_dst_ipv4.setInvalid();
        transition start_0_join;
    }
    state start_0_join {
        transition select(addr_0_ipv6_0.isValid()) {
            true: start_0_true_0;
            false: start_0_false_0;
        }
    }
    state start_0_true_0 {
        hdr.addr_dst_ipv6.setValid();
        hdr.addr_dst_ipv6 = addr_0_ipv6_0;
        hdr.addr_dst_ipv4.setInvalid();
        transition start_0_join_0;
    }
    state start_0_false_0 {
        hdr.addr_dst_ipv6.setInvalid();
        transition start_0_join_0;
    }
    state start_0_join_0 {
        addr_0_ipv4_0.setInvalid();
        addr_0_ipv6_0.setInvalid();
        transition select(hdr.addr_type.srcType) {
            8w0x1: ProtAddrParser_ipv4_0;
            8w0x2: ProtAddrParser_ipv6_0;
            default: noMatch;
        }
    }
    state ProtAddrParser_ipv4_0 {
        packet.extract<addr_ipv4_t>(addr_0_ipv4_0);
        transition start_1;
    }
    state ProtAddrParser_ipv6_0 {
        packet.extract<addr_ipv6_t>(addr_0_ipv6_0);
        transition start_1;
    }
    state start_1 {
        transition select(addr_0_ipv4_0.isValid()) {
            true: start_1_true;
            false: start_1_false;
        }
    }
    state start_1_true {
        hdr.addr_src_ipv4.setValid();
        hdr.addr_src_ipv4 = addr_0_ipv4_0;
        hdr.addr_src_ipv6.setInvalid();
        transition start_1_join;
    }
    state start_1_false {
        hdr.addr_src_ipv4.setInvalid();
        transition start_1_join;
    }
    state start_1_join {
        transition select(addr_0_ipv6_0.isValid()) {
            true: start_1_true_0;
            false: start_1_false_0;
        }
    }
    state start_1_true_0 {
        hdr.addr_src_ipv6.setValid();
        hdr.addr_src_ipv6 = addr_0_ipv6_0;
        hdr.addr_src_ipv4.setInvalid();
        transition start_1_join_0;
    }
    state start_1_false_0 {
        hdr.addr_src_ipv6.setInvalid();
        transition start_1_join_0;
    }
    state start_1_join_0 {
        transition accept;
    }
    state noMatch {
        verify(false, error.NoMatch);
        transition reject;
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
        packet.emit<addr_type_t>(hdr.addr_type);
        packet.emit<addr_ipv4_t>(hdr.addr_dst_ipv4);
        packet.emit<addr_ipv6_t>(hdr.addr_dst_ipv6);
        packet.emit<addr_ipv4_t>(hdr.addr_src_ipv4);
        packet.emit<addr_ipv6_t>(hdr.addr_src_ipv6);
    }
}

V1Switch<headers, metadata>(ProtParser(), ProtVerifyChecksum(), ProtIngress(), ProtEgress(), ProtComputeChecksum(), ProtDeparser()) main;
