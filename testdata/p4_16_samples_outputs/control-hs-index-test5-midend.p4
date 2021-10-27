#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

header ethernet_t {
    bit<48> dst_addr;
    bit<48> src_addr;
    bit<16> eth_type;
}

header h_stack {
    bit<32> a;
    bit<32> b;
    bit<32> c;
}

header h_index {
    bit<32> index;
}

struct headers {
    ethernet_t eth_hdr;
    h_stack[2] h;
    h_index    i;
}

struct Meta {
}

parser p(packet_in pkt, out headers hdr, inout Meta m, inout standard_metadata_t sm) {
    state start {
        pkt.extract<ethernet_t>(hdr.eth_hdr);
        pkt.extract<h_stack>(hdr.h[0]);
        pkt.extract<h_stack>(hdr.h[1]);
        pkt.extract<h_index>(hdr.i);
        transition accept;
    }
}

control ingress(inout headers h, inout Meta m, inout standard_metadata_t sm) {
    bit<32> hsiVar;
    h_stack hsVar1;
    @name("ingress.tmp") bit<32> tmp;
    @name("ingress.hs") h_stack hs_0;
    @name("ingress.stats") counter(32w65536, CounterType.packets) stats_0;
    h_stack hsVar5;
    @name("ingress.add") action add() {
        if (tmp == 32w0) {
            hs_0 = h.h[32w0];
        } else if (tmp == 32w1) {
            hs_0 = h.h[32w1];
        } else {
            h.h[32w1] = hsVar5;
            if (tmp >= 32w1) {
                hs_0 = h.h[32w1];
            }
        }
        hs_0.a = hs_0.a + 32w0xff;
        if (tmp == 32w0) {
            h.h[32w0] = hs_0;
        } else if (tmp == 32w1) {
            h.h[32w1] = hs_0;
        } else {
            h.h[32w1] = hsVar5;
            if (tmp >= 32w1) {
                h.h[32w1] = hs_0;
            }
        }
    }
    @hidden action controlhsindextest5l47() {
        tmp = h.i.index;
    }
    @hidden action controlhsindextest5l48() {
        stats_0.count(h.h[32w0].a);
    }
    @hidden action controlhsindextest5l48_0() {
        stats_0.count(h.h[32w1].a);
    }
    @hidden action controlhsindextest5l48_1() {
        stats_0.count(h.h[32w1].a);
    }
    @hidden action controlhsindextest5l48_2() {
        h.h[32w1] = hsVar1;
    }
    @hidden action controlhsindextest5l48_3() {
        hsiVar = h.i.index;
    }
    @hidden table tbl_controlhsindextest5l47 {
        actions = {
            controlhsindextest5l47();
        }
        const default_action = controlhsindextest5l47();
    }
    @hidden table tbl_add {
        actions = {
            add();
        }
        const default_action = add();
    }
    @hidden table tbl_controlhsindextest5l48 {
        actions = {
            controlhsindextest5l48_3();
        }
        const default_action = controlhsindextest5l48_3();
    }
    @hidden table tbl_controlhsindextest5l48_0 {
        actions = {
            controlhsindextest5l48();
        }
        const default_action = controlhsindextest5l48();
    }
    @hidden table tbl_controlhsindextest5l48_1 {
        actions = {
            controlhsindextest5l48_0();
        }
        const default_action = controlhsindextest5l48_0();
    }
    @hidden table tbl_controlhsindextest5l48_2 {
        actions = {
            controlhsindextest5l48_2();
        }
        const default_action = controlhsindextest5l48_2();
    }
    @hidden table tbl_controlhsindextest5l48_3 {
        actions = {
            controlhsindextest5l48_1();
        }
        const default_action = controlhsindextest5l48_1();
    }
    apply {
        tbl_controlhsindextest5l47.apply();
        tbl_add.apply();
        tbl_controlhsindextest5l48.apply();
        if (hsiVar == 32w0) {
            tbl_controlhsindextest5l48_0.apply();
        } else if (hsiVar == 32w1) {
            tbl_controlhsindextest5l48_1.apply();
        } else {
            tbl_controlhsindextest5l48_2.apply();
            if (hsiVar >= 32w1) {
                tbl_controlhsindextest5l48_3.apply();
            }
        }
    }
}

control vrfy(inout headers h, inout Meta m) {
    apply {
    }
}

control update(inout headers h, inout Meta m) {
    apply {
    }
}

control egress(inout headers h, inout Meta m, inout standard_metadata_t sm) {
    apply {
    }
}

control deparser(packet_out pkt, in headers h) {
    apply {
        pkt.emit<ethernet_t>(h.eth_hdr);
        pkt.emit<h_stack>(h.h[0]);
        pkt.emit<h_stack>(h.h[1]);
        pkt.emit<h_index>(h.i);
    }
}

V1Switch<headers, Meta>(p(), vrfy(), ingress(), egress(), update(), deparser()) main;

