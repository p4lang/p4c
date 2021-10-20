#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

header ethernet_t {
    bit<48> dst_addr;
    bit<48> src_addr;
    bit<16> eth_type;
}

header h_index {
    bit<32> index;
}

header h_stack {
    bit<32> a;
}

struct headers {
    ethernet_t eth_hdr;
    h_stack[3] h;
    h_index    i;
}

struct Meta {
}

parser p(packet_in pkt, out headers hdr, inout Meta m, inout standard_metadata_t sm) {
    state start {
        pkt.extract<ethernet_t>(hdr.eth_hdr);
        pkt.extract<h_stack>(hdr.h[0]);
        pkt.extract<h_stack>(hdr.h[1]);
        pkt.extract<h_stack>(hdr.h[2]);
        pkt.extract<h_index>(hdr.i);
        transition accept;
    }
}

control ingress(inout headers h, inout Meta m, inout standard_metadata_t sm) {
    bit<32> hsiVar0_0;
    h_stack hsVar1;
    @hidden action controlhsindextest2l44() {
        h.h[0].a = 32w1;
    }
    @hidden action controlhsindextest2l44_0() {
        h.h[1].a = 32w1;
    }
    @hidden action controlhsindextest2l44_1() {
        h.h[2].a = 32w1;
    }
    @hidden action controlhsindextest2l44_2() {
        h.h[2].a = 32w1;
    }
    @hidden action controlhsindextest2l43() {
        h.h[32w2] = hsVar1;
    }
    @hidden action controlhsindextest2l43_0() {
        hsiVar0_0 = h.i.index + 32w1;
    }
    @hidden table tbl_controlhsindextest2l43 {
        actions = {
            controlhsindextest2l43_0();
        }
        const default_action = controlhsindextest2l43_0();
    }
    @hidden table tbl_controlhsindextest2l44 {
        actions = {
            controlhsindextest2l44();
        }
        const default_action = controlhsindextest2l44();
    }
    @hidden table tbl_controlhsindextest2l44_0 {
        actions = {
            controlhsindextest2l44_0();
        }
        const default_action = controlhsindextest2l44_0();
    }
    @hidden table tbl_controlhsindextest2l44_1 {
        actions = {
            controlhsindextest2l44_1();
        }
        const default_action = controlhsindextest2l44_1();
    }
    @hidden table tbl_controlhsindextest2l43_0 {
        actions = {
            controlhsindextest2l43();
        }
        const default_action = controlhsindextest2l43();
    }
    @hidden table tbl_controlhsindextest2l44_2 {
        actions = {
            controlhsindextest2l44_2();
        }
        const default_action = controlhsindextest2l44_2();
    }
    apply {
        tbl_controlhsindextest2l43.apply();
        if (hsiVar0_0 == 32w0 && h.h[0].a > 32w10) {
            tbl_controlhsindextest2l44.apply();
        } else if (hsiVar0_0 == 32w1 && h.h[1].a > 32w10) {
            tbl_controlhsindextest2l44_0.apply();
        } else if (hsiVar0_0 == 32w2 && h.h[2].a > 32w10) {
            tbl_controlhsindextest2l44_1.apply();
        } else {
            tbl_controlhsindextest2l43_0.apply();
            if (hsiVar0_0 >= 32w3 && h.h[2].a > 32w10) {
                tbl_controlhsindextest2l44_2.apply();
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
        pkt.emit<h_stack>(h.h[2]);
        pkt.emit<h_index>(h.i);
    }
}

V1Switch<headers, Meta>(p(), vrfy(), ingress(), egress(), update(), deparser()) main;

