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
    bit<32> hsiVar2;
    h_stack hsVar3;
    bit<32> hsiVar4;
    h_stack hsVar5;
    bit<32> hsiVar6;
    h_stack hsVar7;
    bit<32> hsiVar8;
    h_stack hsVar9;
    bit<32> hsiVar10;
    h_stack hsVar11;
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
        h.h[2].a = 32w1;
    }
    @hidden action controlhsindextest3l44_3() {
        h.h[32w2] = hsVar3;
    }
    @hidden action controlhsindextest3l44_4() {
        hsiVar2 = h.h[0].a;
    }
    @hidden action controlhsindextest3l44_5() {
        h.h[0].a = 32w1;
    }
    @hidden action controlhsindextest3l44_6() {
        h.h[1].a = 32w1;
    }
    @hidden action controlhsindextest3l44_7() {
        h.h[2].a = 32w1;
    }
    @hidden action controlhsindextest3l44_8() {
        h.h[2].a = 32w1;
    }
    @hidden action controlhsindextest3l44_9() {
        h.h[32w2] = hsVar5;
    }
    @hidden action controlhsindextest3l44_10() {
        hsiVar4 = h.h[1].a;
    }
    @hidden action controlhsindextest3l44_11() {
        h.h[0].a = 32w1;
    }
    @hidden action controlhsindextest3l44_12() {
        h.h[1].a = 32w1;
    }
    @hidden action controlhsindextest3l44_13() {
        h.h[2].a = 32w1;
    }
    @hidden action controlhsindextest3l44_14() {
        h.h[2].a = 32w1;
    }
    @hidden action controlhsindextest3l44_15() {
        h.h[32w2] = hsVar7;
    }
    @hidden action controlhsindextest3l44_16() {
        hsiVar6 = h.h[2].a;
    }
    @hidden action controlhsindextest3l44_17() {
        h.h[0].a = 32w1;
    }
    @hidden action controlhsindextest3l44_18() {
        h.h[1].a = 32w1;
    }
    @hidden action controlhsindextest3l44_19() {
        h.h[2].a = 32w1;
    }
    @hidden action controlhsindextest3l44_20() {
        h.h[2].a = 32w1;
    }
    @hidden action controlhsindextest3l44_21() {
        h.h[32w2] = hsVar9;
    }
    @hidden action controlhsindextest3l44_22() {
        hsiVar8 = h.h[2].a;
    }
    @hidden action controlhsindextest3l43() {
        h.h[32w2] = hsVar1;
    }
    @hidden action controlhsindextest3l43_0() {
        hsiVar0_0 = h.i.index;
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
        h.h[2].setInvalid();
    }
    @hidden action controlhsindextest3l45_3() {
        h.h[32w2] = hsVar11;
    }
    @hidden action controlhsindextest3l45_4() {
        hsiVar10 = h.i.index;
    }
    @hidden table tbl_controlhsindextest3l43 {
        actions = {
            controlhsindextest3l43_0();
        }
        const default_action = controlhsindextest3l43_0();
    }
    @hidden table tbl_controlhsindextest3l44 {
        actions = {
            controlhsindextest3l44_4();
        }
        const default_action = controlhsindextest3l44_4();
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
            controlhsindextest3l44_3();
        }
        const default_action = controlhsindextest3l44_3();
    }
    @hidden table tbl_controlhsindextest3l44_4 {
        actions = {
            controlhsindextest3l44_2();
        }
        const default_action = controlhsindextest3l44_2();
    }
    @hidden table tbl_controlhsindextest3l44_5 {
        actions = {
            controlhsindextest3l44_10();
        }
        const default_action = controlhsindextest3l44_10();
    }
    @hidden table tbl_controlhsindextest3l44_6 {
        actions = {
            controlhsindextest3l44_5();
        }
        const default_action = controlhsindextest3l44_5();
    }
    @hidden table tbl_controlhsindextest3l44_7 {
        actions = {
            controlhsindextest3l44_6();
        }
        const default_action = controlhsindextest3l44_6();
    }
    @hidden table tbl_controlhsindextest3l44_8 {
        actions = {
            controlhsindextest3l44_7();
        }
        const default_action = controlhsindextest3l44_7();
    }
    @hidden table tbl_controlhsindextest3l44_9 {
        actions = {
            controlhsindextest3l44_9();
        }
        const default_action = controlhsindextest3l44_9();
    }
    @hidden table tbl_controlhsindextest3l44_10 {
        actions = {
            controlhsindextest3l44_8();
        }
        const default_action = controlhsindextest3l44_8();
    }
    @hidden table tbl_controlhsindextest3l44_11 {
        actions = {
            controlhsindextest3l44_16();
        }
        const default_action = controlhsindextest3l44_16();
    }
    @hidden table tbl_controlhsindextest3l44_12 {
        actions = {
            controlhsindextest3l44_11();
        }
        const default_action = controlhsindextest3l44_11();
    }
    @hidden table tbl_controlhsindextest3l44_13 {
        actions = {
            controlhsindextest3l44_12();
        }
        const default_action = controlhsindextest3l44_12();
    }
    @hidden table tbl_controlhsindextest3l44_14 {
        actions = {
            controlhsindextest3l44_13();
        }
        const default_action = controlhsindextest3l44_13();
    }
    @hidden table tbl_controlhsindextest3l44_15 {
        actions = {
            controlhsindextest3l44_15();
        }
        const default_action = controlhsindextest3l44_15();
    }
    @hidden table tbl_controlhsindextest3l44_16 {
        actions = {
            controlhsindextest3l44_14();
        }
        const default_action = controlhsindextest3l44_14();
    }
    @hidden table tbl_controlhsindextest3l43_0 {
        actions = {
            controlhsindextest3l43();
        }
        const default_action = controlhsindextest3l43();
    }
    @hidden table tbl_controlhsindextest3l44_17 {
        actions = {
            controlhsindextest3l44_22();
        }
        const default_action = controlhsindextest3l44_22();
    }
    @hidden table tbl_controlhsindextest3l44_18 {
        actions = {
            controlhsindextest3l44_17();
        }
        const default_action = controlhsindextest3l44_17();
    }
    @hidden table tbl_controlhsindextest3l44_19 {
        actions = {
            controlhsindextest3l44_18();
        }
        const default_action = controlhsindextest3l44_18();
    }
    @hidden table tbl_controlhsindextest3l44_20 {
        actions = {
            controlhsindextest3l44_19();
        }
        const default_action = controlhsindextest3l44_19();
    }
    @hidden table tbl_controlhsindextest3l44_21 {
        actions = {
            controlhsindextest3l44_21();
        }
        const default_action = controlhsindextest3l44_21();
    }
    @hidden table tbl_controlhsindextest3l44_22 {
        actions = {
            controlhsindextest3l44_20();
        }
        const default_action = controlhsindextest3l44_20();
    }
    @hidden table tbl_controlhsindextest3l45 {
        actions = {
            controlhsindextest3l45_4();
        }
        const default_action = controlhsindextest3l45_4();
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
    @hidden table tbl_controlhsindextest3l45_3 {
        actions = {
            controlhsindextest3l45_3();
        }
        const default_action = controlhsindextest3l45_3();
    }
    @hidden table tbl_controlhsindextest3l45_4 {
        actions = {
            controlhsindextest3l45_2();
        }
        const default_action = controlhsindextest3l45_2();
    }
    apply {
        tbl_controlhsindextest3l43.apply();
        if (hsiVar0_0 == 32w0 && h.h[0].isValid()) {
            tbl_controlhsindextest3l44.apply();
            if (hsiVar2 == 32w0) {
                tbl_controlhsindextest3l44_0.apply();
            } else if (hsiVar2 == 32w1) {
                tbl_controlhsindextest3l44_1.apply();
            } else if (hsiVar2 == 32w2) {
                tbl_controlhsindextest3l44_2.apply();
            } else {
                tbl_controlhsindextest3l44_3.apply();
                if (hsiVar2 >= 32w3) {
                    tbl_controlhsindextest3l44_4.apply();
                }
            }
        } else if (hsiVar0_0 == 32w1 && h.h[1].isValid()) {
            tbl_controlhsindextest3l44_5.apply();
            if (hsiVar4 == 32w0) {
                tbl_controlhsindextest3l44_6.apply();
            } else if (hsiVar4 == 32w1) {
                tbl_controlhsindextest3l44_7.apply();
            } else if (hsiVar4 == 32w2) {
                tbl_controlhsindextest3l44_8.apply();
            } else {
                tbl_controlhsindextest3l44_9.apply();
                if (hsiVar4 >= 32w3) {
                    tbl_controlhsindextest3l44_10.apply();
                }
            }
        } else if (hsiVar0_0 == 32w2 && h.h[2].isValid()) {
            tbl_controlhsindextest3l44_11.apply();
            if (hsiVar6 == 32w0) {
                tbl_controlhsindextest3l44_12.apply();
            } else if (hsiVar6 == 32w1) {
                tbl_controlhsindextest3l44_13.apply();
            } else if (hsiVar6 == 32w2) {
                tbl_controlhsindextest3l44_14.apply();
            } else {
                tbl_controlhsindextest3l44_15.apply();
                if (hsiVar6 >= 32w3) {
                    tbl_controlhsindextest3l44_16.apply();
                }
            }
        } else {
            tbl_controlhsindextest3l43_0.apply();
            if (hsiVar0_0 >= 32w3 && h.h[2].isValid()) {
                tbl_controlhsindextest3l44_17.apply();
                if (hsiVar8 == 32w0) {
                    tbl_controlhsindextest3l44_18.apply();
                } else if (hsiVar8 == 32w1) {
                    tbl_controlhsindextest3l44_19.apply();
                } else if (hsiVar8 == 32w2) {
                    tbl_controlhsindextest3l44_20.apply();
                } else {
                    tbl_controlhsindextest3l44_21.apply();
                    if (hsiVar8 >= 32w3) {
                        tbl_controlhsindextest3l44_22.apply();
                    }
                }
            }
        }
        tbl_controlhsindextest3l45.apply();
        if (hsiVar10 == 32w0) {
            tbl_controlhsindextest3l45_0.apply();
        } else if (hsiVar10 == 32w1) {
            tbl_controlhsindextest3l45_1.apply();
        } else if (hsiVar10 == 32w2) {
            tbl_controlhsindextest3l45_2.apply();
        } else {
            tbl_controlhsindextest3l45_3.apply();
            if (hsiVar10 >= 32w3) {
                tbl_controlhsindextest3l45_4.apply();
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

