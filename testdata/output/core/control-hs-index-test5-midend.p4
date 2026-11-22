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
    @name("ingress.tmp") bit<32> tmp;
    @name("ingress.hs") h_stack hs_0;
    @name("ingress.stats") counter(32w65536, CounterType.packets) stats_0;
    h_stack hsVar;
    @name("ingress.add") action add() {
        if (tmp == 32w0) {
            hs_0 = h.h[32w0];
        } else if (tmp == 32w1) {
            hs_0 = h.h[32w1];
        } else if (tmp >= 32w1) {
            hs_0 = hsVar;
        }
        hs_0.a = hs_0.a + 32w0xff;
        if (tmp == 32w0) {
            h.h[32w0] = hs_0;
        } else if (tmp == 32w1) {
            h.h[32w1] = hs_0;
        }
    }
    @hidden action controlhsindextest5l49() {
        tmp = h.i.index;
    }
    @hidden action controlhsindextest5l50() {
        stats_0.count(h.h[32w0].a);
    }
    @hidden action controlhsindextest5l50_0() {
        stats_0.count(h.h[32w1].a);
    }
    @hidden action controlhsindextest5l50_1() {
        hsiVar = h.i.index;
    }
    @hidden table tbl_controlhsindextest5l49 {
        actions = {
            controlhsindextest5l49();
        }
        const default_action = controlhsindextest5l49();
    }
    @hidden table tbl_add {
        actions = {
            add();
        }
        const default_action = add();
    }
    @hidden table tbl_controlhsindextest5l50 {
        actions = {
            controlhsindextest5l50_1();
        }
        const default_action = controlhsindextest5l50_1();
    }
    @hidden table tbl_controlhsindextest5l50_0 {
        actions = {
            controlhsindextest5l50();
        }
        const default_action = controlhsindextest5l50();
    }
    @hidden table tbl_controlhsindextest5l50_1 {
        actions = {
            controlhsindextest5l50_0();
        }
        const default_action = controlhsindextest5l50_0();
    }
    apply {
        tbl_controlhsindextest5l49.apply();
        tbl_add.apply();
        tbl_controlhsindextest5l50.apply();
        if (hsiVar == 32w0) {
            tbl_controlhsindextest5l50_0.apply();
        } else if (hsiVar == 32w1) {
            tbl_controlhsindextest5l50_1.apply();
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
