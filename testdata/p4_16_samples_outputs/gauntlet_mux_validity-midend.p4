#include <core.p4>

header ethernet_t {
    bit<48> dst_addr;
    bit<48> src_addr;
    bit<16> eth_type;
}

header H {
    bit<8> a;
}

struct Headers {
    ethernet_t eth_hdr;
    H          h;
}

parser p(packet_in pkt, out Headers hdr) {
    state start {
        pkt.extract<ethernet_t>(hdr.eth_hdr);
        pkt.extract<H>(hdr.h);
        transition accept;
    }
}

control ingress(inout Headers h) {
    @name("ingress.b") bool b_0;
    @name("ingress.tmp") bit<16> tmp;
    @name("ingress.tmp_0") bit<16> tmp_0;
    @name("ingress.dummy_1") H dummy_1;
    @name("ingress.hasReturned") bool hasReturned;
    @name("ingress.retval") bit<16> retval;
    @name("ingress.dummy") action dummy_2() {
        dummy_1 = (b_0 ? h.h : dummy_1);
        hasReturned = (b_0 ? true : hasReturned);
        retval = (b_0 ? 16w1 : retval);
        h.h = (b_0 ? dummy_1 : h.h);
        tmp_0 = (b_0 ? retval : tmp_0);
        tmp = (b_0 ? tmp_0 : tmp);
        tmp = (b_0 ? tmp_0 : 16w1);
        h.eth_hdr.eth_type = (b_0 ? tmp_0 : 16w1);
    }
    @hidden action gauntlet_mux_validity36() {
        b_0 = false;
    }
    @hidden table tbl_gauntlet_mux_validity36 {
        actions = {
            gauntlet_mux_validity36();
        }
        const default_action = gauntlet_mux_validity36();
    }
    @hidden table tbl_dummy {
        actions = {
            dummy_2();
        }
        const default_action = dummy_2();
    }
    apply {
        tbl_gauntlet_mux_validity36.apply();
        tbl_dummy.apply();
    }
}

parser Parser(packet_in b, out Headers hdr);
control Ingress(inout Headers hdr);
package top(Parser p, Ingress ig);
top(p(), ingress()) main;

