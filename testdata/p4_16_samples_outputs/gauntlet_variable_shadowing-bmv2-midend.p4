#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

header ethernet_t {
    bit<48> dst_addr;
    bit<48> src_addr;
    bit<16> eth_type;
}

header H {
    bit<32> a;
    bit<32> b;
    bit<8>  c;
}

struct Headers {
    ethernet_t eth_hdr;
    H          h;
}

struct Meta {
    bit<8> test;
}

parser p(packet_in pkt, out Headers h, inout Meta m, inout standard_metadata_t sm) {
    state start {
        pkt.extract<ethernet_t>(h.eth_hdr);
        pkt.extract<H>(h.h);
        transition accept;
    }
}

control ingress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    bit<32> key_0;
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @name("ingress.c.a") action c_a_0() {
        h.h.b = h.h.a;
    }
    @name("ingress.c.t") table c_t {
        key = {
            key_0: exact @name("e");
        }
        actions = {
            c_a_0();
            NoAction_1();
        }
        default_action = NoAction_1();
    }
    @hidden action gauntlet_variable_shadowingbmv2l31() {
        m.test = 8w1;
        key_0 = h.h.a + h.h.a;
    }
    @hidden action gauntlet_variable_shadowingbmv2l62() {
        h.h.c = 8w1;
    }
    @hidden table tbl_gauntlet_variable_shadowingbmv2l31 {
        actions = {
            gauntlet_variable_shadowingbmv2l31();
        }
        const default_action = gauntlet_variable_shadowingbmv2l31();
    }
    @hidden table tbl_gauntlet_variable_shadowingbmv2l62 {
        actions = {
            gauntlet_variable_shadowingbmv2l62();
        }
        const default_action = gauntlet_variable_shadowingbmv2l62();
    }
    apply {
        tbl_gauntlet_variable_shadowingbmv2l31.apply();
        c_t.apply();
        tbl_gauntlet_variable_shadowingbmv2l62.apply();
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
