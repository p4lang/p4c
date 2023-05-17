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
    @name("p.e") ethernet_t e_0;
    state start {
        e_0.setInvalid();
        pkt.extract<ethernet_t>(e_0);
        transition select(e_0.eth_type) {
            16w0x800: accept;
            16w0x806: accept;
            default: reject;
        }
    }
}

control ingress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    @name("ingress.reg") register<bit<16>>(32w1) reg_0;
    @hidden action registerserenumbmv2l44() {
        reg_0.write(32w0, h.eth_hdr.eth_type);
    }
    @hidden table tbl_registerserenumbmv2l44 {
        actions = {
            registerserenumbmv2l44();
        }
        const default_action = registerserenumbmv2l44();
    }
    apply {
        tbl_registerserenumbmv2l44.apply();
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
        pkt.emit<ethernet_t>(h.eth_hdr);
    }
}

V1Switch<Headers, Meta>(p(), vrfy(), ingress(), egress(), update(), deparser()) main;
