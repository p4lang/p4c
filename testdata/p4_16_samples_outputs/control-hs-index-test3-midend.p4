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
    bit<32> hsiVar0;
    bit<32> hsiVar1;
    bit<32> hsiVar2;
    bit<32> hsiVar3;
    bit<32> hsiVar4;
    @hidden action controlhsindextest3l44() {
        h.h[0].a = 32w1;
    }
    @hidden action controlhsindextest3l44_0() {
        h.h[1].a = 32w1;
    }
    @hidden action controlhsindextest3l44_1() {
        h.h[2].a = 32w1;
    }
    @hidden action controlhsindextest3l44_2() {
        hsiVar1 = h.h[0].a;
    }
    @hidden action controlhsindextest3l44_3() {
        h.h[0].a = 32w1;
    }
    @hidden action controlhsindextest3l44_4() {
        h.h[1].a = 32w1;
    }
    @hidden action controlhsindextest3l44_5() {
        h.h[2].a = 32w1;
    }
    @hidden action controlhsindextest3l44_6() {
        hsiVar2 = h.h[1].a;
    }
    @hidden action controlhsindextest3l44_7() {
        h.h[0].a = 32w1;
    }
    @hidden action controlhsindextest3l44_8() {
        h.h[1].a = 32w1;
    }
    @hidden action controlhsindextest3l44_9() {
        h.h[2].a = 32w1;
    }
    @hidden action controlhsindextest3l44_10() {
        hsiVar3 = h.h[2].a;
    }
    @hidden action controlhsindextest3l43() {
        hsiVar0 = h.i.index;
    }
    @hidden action controlhsindextest3l45() {
        h.h[0].setInvalid();
    }
    @hidden action controlhsindextest3l45_0() {
        h.h[1].setInvalid();
    }
    @hidden action controlhsindextest3l45_1() {
        h.h[2].setInvalid();
    }
    @hidden action controlhsindextest3l45_2() {
        hsiVar4 = h.i.index;
    }
    @hidden table tbl_controlhsindextest3l43 {
        actions = {
            controlhsindextest3l43();
        }
        const default_action = controlhsindextest3l43();
    }
    @hidden table tbl_controlhsindextest3l44 {
        actions = {
            controlhsindextest3l44_2();
        }
        const default_action = controlhsindextest3l44_2();
    }
    @hidden table tbl_controlhsindextest3l44_0 {
        actions = {
            controlhsindextest3l44();
        }
        const default_action = controlhsindextest3l44();
    }
    @hidden table tbl_controlhsindextest3l44_1 {
        actions = {
            controlhsindextest3l44_0();
        }
        const default_action = controlhsindextest3l44_0();
    }
    @hidden table tbl_controlhsindextest3l44_2 {
        actions = {
            controlhsindextest3l44_1();
        }
        const default_action = controlhsindextest3l44_1();
    }
    @hidden table tbl_controlhsindextest3l44_3 {
        actions = {
            controlhsindextest3l44_6();
        }
        const default_action = controlhsindextest3l44_6();
    }
    @hidden table tbl_controlhsindextest3l44_4 {
        actions = {
            controlhsindextest3l44_3();
        }
        const default_action = controlhsindextest3l44_3();
    }
    @hidden table tbl_controlhsindextest3l44_5 {
        actions = {
            controlhsindextest3l44_4();
        }
        const default_action = controlhsindextest3l44_4();
    }
    @hidden table tbl_controlhsindextest3l44_6 {
        actions = {
            controlhsindextest3l44_5();
        }
        const default_action = controlhsindextest3l44_5();
    }
    @hidden table tbl_controlhsindextest3l44_7 {
        actions = {
            controlhsindextest3l44_10();
        }
        const default_action = controlhsindextest3l44_10();
    }
    @hidden table tbl_controlhsindextest3l44_8 {
        actions = {
            controlhsindextest3l44_7();
        }
        const default_action = controlhsindextest3l44_7();
    }
    @hidden table tbl_controlhsindextest3l44_9 {
        actions = {
            controlhsindextest3l44_8();
        }
        const default_action = controlhsindextest3l44_8();
    }
    @hidden table tbl_controlhsindextest3l44_10 {
        actions = {
            controlhsindextest3l44_9();
        }
        const default_action = controlhsindextest3l44_9();
    }
    @hidden table tbl_controlhsindextest3l45 {
        actions = {
            controlhsindextest3l45_2();
        }
        const default_action = controlhsindextest3l45_2();
    }
    @hidden table tbl_controlhsindextest3l45_0 {
        actions = {
            controlhsindextest3l45();
        }
        const default_action = controlhsindextest3l45();
    }
    @hidden table tbl_controlhsindextest3l45_1 {
        actions = {
            controlhsindextest3l45_0();
        }
        const default_action = controlhsindextest3l45_0();
    }
    @hidden table tbl_controlhsindextest3l45_2 {
        actions = {
            controlhsindextest3l45_1();
        }
        const default_action = controlhsindextest3l45_1();
    }
    apply {
        tbl_controlhsindextest3l43.apply();
        if (hsiVar0 == 32w0 && h.h[0].isValid()) {
            tbl_controlhsindextest3l44.apply();
            if (hsiVar1 == 32w0) {
                tbl_controlhsindextest3l44_0.apply();
            } else if (hsiVar1 == 32w1) {
                tbl_controlhsindextest3l44_1.apply();
            } else if (hsiVar1 == 32w2) {
                tbl_controlhsindextest3l44_2.apply();
            }
        } else if (hsiVar0 == 32w1 && h.h[1].isValid()) {
            tbl_controlhsindextest3l44_3.apply();
            if (hsiVar2 == 32w0) {
                tbl_controlhsindextest3l44_4.apply();
            } else if (hsiVar2 == 32w1) {
                tbl_controlhsindextest3l44_5.apply();
            } else if (hsiVar2 == 32w2) {
                tbl_controlhsindextest3l44_6.apply();
            }
        } else if (hsiVar0 == 32w2 && h.h[2].isValid()) {
            tbl_controlhsindextest3l44_7.apply();
            if (hsiVar3 == 32w0) {
                tbl_controlhsindextest3l44_8.apply();
            } else if (hsiVar3 == 32w1) {
                tbl_controlhsindextest3l44_9.apply();
            } else if (hsiVar3 == 32w2) {
                tbl_controlhsindextest3l44_10.apply();
            }
        }
        tbl_controlhsindextest3l45.apply();
        if (hsiVar4 == 32w0) {
            tbl_controlhsindextest3l45_0.apply();
        } else if (hsiVar4 == 32w1) {
            tbl_controlhsindextest3l45_1.apply();
        } else if (hsiVar4 == 32w2) {
            tbl_controlhsindextest3l45_2.apply();
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

