#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

header ethernet_t {
    bit<48> dst_addr;
    bit<48> src_addr;
    bit<16> eth_type;
}

header H {
    bit<8> a;
    bit<8> b;
}

struct Headers {
    ethernet_t eth_hdr;
    H          h;
}

struct Meta {
}

control ingress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    @name("ingress.c") bit<8> c_0;
    @name("ingress.do_thing") action do_thing() {
        h.h.a = c_0;
    }
    @name("ingress.do_thing") action do_thing_1() {
        h.h.a = c_0;
    }
    @hidden action gauntlet_copy_outbmv2l23() {
        c_0 = 8w12;
    }
    @hidden table tbl_gauntlet_copy_outbmv2l23 {
        actions = {
            gauntlet_copy_outbmv2l23();
        }
        const default_action = gauntlet_copy_outbmv2l23();
    }
    @hidden table tbl_do_thing {
        actions = {
            do_thing();
        }
        const default_action = do_thing();
    }
    @hidden table tbl_do_thing_0 {
        actions = {
            do_thing_1();
        }
        const default_action = do_thing_1();
    }
    apply {
        tbl_gauntlet_copy_outbmv2l23.apply();
        tbl_do_thing.apply();
        tbl_do_thing_0.apply();
    }
}

parser p(packet_in pkt, out Headers hdr, inout Meta m, inout standard_metadata_t sm) {
    state start {
        pkt.extract<ethernet_t>(hdr.eth_hdr);
        pkt.extract<H>(hdr.h);
        transition accept;
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
        pkt.emit<H>(h.h);
    }
}

V1Switch<Headers, Meta>(p(), vrfy(), ingress(), egress(), update(), deparser()) main;

