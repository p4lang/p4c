#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

header ethernet_t {
    bit<48> dst_addr;
    bit<48> src_addr;
    bit<16> eth_type;
}

struct Headers {
    ethernet_t eth_hdr;
}

struct Meta {
}

parser p(packet_in pkt, out Headers hdr, inout Meta m, inout standard_metadata_t sm) {
    state start {
        pkt.extract<ethernet_t>(hdr.eth_hdr);
        transition accept;
    }
}

control ingress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    @name("ingress.addr_0") bit<48> addr;
    @name("ingress.option_0") bool option;
    @name("ingress.addr_1") bit<48> addr_2;
    @name("ingress.option_1") bool option_2;
    apply {
        addr = h.eth_hdr.dst_addr;
        option = true;
        if (option) {
            addr = 48w0xaabbccddeeff;
        }
        h.eth_hdr.dst_addr = addr;
        addr_2 = h.eth_hdr.src_addr;
        option_2 = false;
        if (option_2) {
            addr_2 = 48w0xaabbccddeeff;
        }
        h.eth_hdr.src_addr = addr_2;
    }
}

control vrfy(inout Headers h, inout Meta m) {
    apply {
    }
}

control update(inout Headers h, inout Meta m) {
    apply {
    }
}

control egress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    apply {
    }
}

control deparser(packet_out pkt, in Headers h) {
    apply {
        pkt.emit<Headers>(h);
    }
}

V1Switch<Headers, Meta>(p(), vrfy(), ingress(), egress(), update(), deparser()) main;
