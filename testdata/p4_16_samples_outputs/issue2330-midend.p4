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
    bit<48> tmp;
    bit<16> val_0;
    @name("ingress.do_action") action do_action() {
        val_0 = (!(h.eth_hdr.dst_addr != 48w0 ? false : true) ? h.eth_hdr.eth_type : val_0);
        h.eth_hdr.eth_type = (!(h.eth_hdr.dst_addr != 48w0 ? false : true) ? val_0 : h.eth_hdr.eth_type);
        tmp = (!(h.eth_hdr.dst_addr != 48w0 ? false : true) ? 48w1 : tmp);
        tmp = (!(h.eth_hdr.dst_addr != 48w0 ? false : true) ? tmp : tmp);
        h.eth_hdr.src_addr = (!(h.eth_hdr.dst_addr != 48w0 ? false : true) ? tmp : h.eth_hdr.src_addr);
    }
    @hidden table tbl_do_action {
        actions = {
            do_action();
        }
        const default_action = do_action();
    }
    apply {
        tbl_do_action.apply();
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

