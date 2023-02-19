#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

header ethernet_t {
    bit<48> dst_addr;
    bit<48> src_addr;
    bit<16> eth_type;
}

header h_index {
    bit<32> index1;
    bit<32> index2;
    bit<32> index3;
}

header h_stack {
    bit<32> a;
    bit<32> b;
    bit<32> c;
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
    bit<32> hsiVar;
    bit<32> hsiVar_0;
    bit<32> hsiVar_1;
    @hidden action controlhsindextest4l49() {
        h.h[0].a = 32w0;
    }
    @hidden action controlhsindextest4l48() {
        hsiVar_1 = h.i.index3;
    }
    @hidden action controlhsindextest4l48_0() {
        hsiVar_1 = h.i.index3;
    }
    @hidden action controlhsindextest4l48_1() {
        hsiVar_1 = h.i.index3;
    }
    @hidden action controlhsindextest4l48_2() {
        hsiVar_0 = h.i.index2;
    }
    @hidden action controlhsindextest4l48_3() {
        hsiVar_1 = h.i.index3;
    }
    @hidden action controlhsindextest4l48_4() {
        hsiVar_1 = h.i.index3;
    }
    @hidden action controlhsindextest4l48_5() {
        hsiVar_1 = h.i.index3;
    }
    @hidden action controlhsindextest4l48_6() {
        hsiVar_0 = h.i.index2;
    }
    @hidden action controlhsindextest4l48_7() {
        hsiVar_1 = h.i.index3;
    }
    @hidden action controlhsindextest4l48_8() {
        hsiVar_1 = h.i.index3;
    }
    @hidden action controlhsindextest4l48_9() {
        hsiVar_1 = h.i.index3;
    }
    @hidden action controlhsindextest4l48_10() {
        hsiVar_0 = h.i.index2;
    }
    @hidden action controlhsindextest4l48_11() {
        hsiVar = h.i.index1;
    }
    @hidden table tbl_controlhsindextest4l48 {
        actions = {
            controlhsindextest4l48_11();
        }
        const default_action = controlhsindextest4l48_11();
    }
    @hidden table tbl_controlhsindextest4l48_0 {
        actions = {
            controlhsindextest4l48_10();
        }
        const default_action = controlhsindextest4l48_10();
    }
    @hidden table tbl_controlhsindextest4l48_1 {
        actions = {
            controlhsindextest4l48_9();
        }
        const default_action = controlhsindextest4l48_9();
    }
    @hidden table tbl_controlhsindextest4l49 {
        actions = {
            controlhsindextest4l49();
        }
        const default_action = controlhsindextest4l49();
    }
    @hidden table tbl_controlhsindextest4l48_2 {
        actions = {
            controlhsindextest4l48_8();
        }
        const default_action = controlhsindextest4l48_8();
    }
    @hidden table tbl_controlhsindextest4l48_3 {
        actions = {
            controlhsindextest4l48_7();
        }
        const default_action = controlhsindextest4l48_7();
    }
    @hidden table tbl_controlhsindextest4l48_4 {
        actions = {
            controlhsindextest4l48_6();
        }
        const default_action = controlhsindextest4l48_6();
    }
    @hidden table tbl_controlhsindextest4l48_5 {
        actions = {
            controlhsindextest4l48_5();
        }
        const default_action = controlhsindextest4l48_5();
    }
    @hidden table tbl_controlhsindextest4l48_6 {
        actions = {
            controlhsindextest4l48_4();
        }
        const default_action = controlhsindextest4l48_4();
    }
    @hidden table tbl_controlhsindextest4l48_7 {
        actions = {
            controlhsindextest4l48_3();
        }
        const default_action = controlhsindextest4l48_3();
    }
    @hidden table tbl_controlhsindextest4l48_8 {
        actions = {
            controlhsindextest4l48_2();
        }
        const default_action = controlhsindextest4l48_2();
    }
    @hidden table tbl_controlhsindextest4l48_9 {
        actions = {
            controlhsindextest4l48_1();
        }
        const default_action = controlhsindextest4l48_1();
    }
    @hidden table tbl_controlhsindextest4l48_10 {
        actions = {
            controlhsindextest4l48_0();
        }
        const default_action = controlhsindextest4l48_0();
    }
    @hidden table tbl_controlhsindextest4l48_11 {
        actions = {
            controlhsindextest4l48();
        }
        const default_action = controlhsindextest4l48();
    }
    apply {
        tbl_controlhsindextest4l48.apply();
        tbl_controlhsindextest4l48_0.apply();
        tbl_controlhsindextest4l48_1.apply();
        if (hsiVar_1 == 32w0 && (hsiVar_0 == 32w0 && (hsiVar == 32w0 && h.h[32w0].a + h.h[32w0].b + h.h[32w0].c > 32w20))) {
            tbl_controlhsindextest4l49.apply();
        } else if (hsiVar_1 == 32w1 && (hsiVar_0 == 32w0 && (hsiVar == 32w0 && h.h[32w0].a + h.h[32w0].b + h.h[32w1].c > 32w20))) {
            tbl_controlhsindextest4l49.apply();
        } else if (hsiVar_1 == 32w2 && (hsiVar_0 == 32w0 && (hsiVar == 32w0 && h.h[32w0].a + h.h[32w0].b + h.h[32w2].c > 32w20))) {
            tbl_controlhsindextest4l49.apply();
        } else {
            tbl_controlhsindextest4l48_2.apply();
            if (hsiVar_1 == 32w0 && (hsiVar_0 == 32w1 && (hsiVar == 32w0 && h.h[32w0].a + h.h[32w1].b + h.h[32w0].c > 32w20))) {
                tbl_controlhsindextest4l49.apply();
            } else if (hsiVar_1 == 32w1 && (hsiVar_0 == 32w1 && (hsiVar == 32w0 && h.h[32w0].a + h.h[32w1].b + h.h[32w1].c > 32w20))) {
                tbl_controlhsindextest4l49.apply();
            } else if (hsiVar_1 == 32w2 && (hsiVar_0 == 32w1 && (hsiVar == 32w0 && h.h[32w0].a + h.h[32w1].b + h.h[32w2].c > 32w20))) {
                tbl_controlhsindextest4l49.apply();
            } else {
                tbl_controlhsindextest4l48_3.apply();
                if (hsiVar_1 == 32w0 && (hsiVar_0 == 32w2 && (hsiVar == 32w0 && h.h[32w0].a + h.h[32w2].b + h.h[32w0].c > 32w20))) {
                    tbl_controlhsindextest4l49.apply();
                } else if (hsiVar_1 == 32w1 && (hsiVar_0 == 32w2 && (hsiVar == 32w0 && h.h[32w0].a + h.h[32w2].b + h.h[32w1].c > 32w20))) {
                    tbl_controlhsindextest4l49.apply();
                } else if (hsiVar_1 == 32w2 && (hsiVar_0 == 32w2 && (hsiVar == 32w0 && h.h[32w0].a + h.h[32w2].b + h.h[32w2].c > 32w20))) {
                    tbl_controlhsindextest4l49.apply();
                } else {
                    tbl_controlhsindextest4l48_4.apply();
                    tbl_controlhsindextest4l48_5.apply();
                    if (hsiVar_1 == 32w0 && (hsiVar_0 == 32w0 && (hsiVar == 32w1 && h.h[32w1].a + h.h[32w0].b + h.h[32w0].c > 32w20))) {
                        tbl_controlhsindextest4l49.apply();
                    } else if (hsiVar_1 == 32w1 && (hsiVar_0 == 32w0 && (hsiVar == 32w1 && h.h[32w1].a + h.h[32w0].b + h.h[32w1].c > 32w20))) {
                        tbl_controlhsindextest4l49.apply();
                    } else if (hsiVar_1 == 32w2 && (hsiVar_0 == 32w0 && (hsiVar == 32w1 && h.h[32w1].a + h.h[32w0].b + h.h[32w2].c > 32w20))) {
                        tbl_controlhsindextest4l49.apply();
                    } else {
                        tbl_controlhsindextest4l48_6.apply();
                        if (hsiVar_1 == 32w0 && (hsiVar_0 == 32w1 && (hsiVar == 32w1 && h.h[32w1].a + h.h[32w1].b + h.h[32w0].c > 32w20))) {
                            tbl_controlhsindextest4l49.apply();
                        } else if (hsiVar_1 == 32w1 && (hsiVar_0 == 32w1 && (hsiVar == 32w1 && h.h[32w1].a + h.h[32w1].b + h.h[32w1].c > 32w20))) {
                            tbl_controlhsindextest4l49.apply();
                        } else if (hsiVar_1 == 32w2 && (hsiVar_0 == 32w1 && (hsiVar == 32w1 && h.h[32w1].a + h.h[32w1].b + h.h[32w2].c > 32w20))) {
                            tbl_controlhsindextest4l49.apply();
                        } else {
                            tbl_controlhsindextest4l48_7.apply();
                            if (hsiVar_1 == 32w0 && (hsiVar_0 == 32w2 && (hsiVar == 32w1 && h.h[32w1].a + h.h[32w2].b + h.h[32w0].c > 32w20))) {
                                tbl_controlhsindextest4l49.apply();
                            } else if (hsiVar_1 == 32w1 && (hsiVar_0 == 32w2 && (hsiVar == 32w1 && h.h[32w1].a + h.h[32w2].b + h.h[32w1].c > 32w20))) {
                                tbl_controlhsindextest4l49.apply();
                            } else if (hsiVar_1 == 32w2 && (hsiVar_0 == 32w2 && (hsiVar == 32w1 && h.h[32w1].a + h.h[32w2].b + h.h[32w2].c > 32w20))) {
                                tbl_controlhsindextest4l49.apply();
                            } else {
                                tbl_controlhsindextest4l48_8.apply();
                                tbl_controlhsindextest4l48_9.apply();
                                if (hsiVar_1 == 32w0 && (hsiVar_0 == 32w0 && (hsiVar == 32w2 && h.h[32w2].a + h.h[32w0].b + h.h[32w0].c > 32w20))) {
                                    tbl_controlhsindextest4l49.apply();
                                } else if (hsiVar_1 == 32w1 && (hsiVar_0 == 32w0 && (hsiVar == 32w2 && h.h[32w2].a + h.h[32w0].b + h.h[32w1].c > 32w20))) {
                                    tbl_controlhsindextest4l49.apply();
                                } else if (hsiVar_1 == 32w2 && (hsiVar_0 == 32w0 && (hsiVar == 32w2 && h.h[32w2].a + h.h[32w0].b + h.h[32w2].c > 32w20))) {
                                    tbl_controlhsindextest4l49.apply();
                                } else {
                                    tbl_controlhsindextest4l48_10.apply();
                                    if (hsiVar_1 == 32w0 && (hsiVar_0 == 32w1 && (hsiVar == 32w2 && h.h[32w2].a + h.h[32w1].b + h.h[32w0].c > 32w20))) {
                                        tbl_controlhsindextest4l49.apply();
                                    } else if (hsiVar_1 == 32w1 && (hsiVar_0 == 32w1 && (hsiVar == 32w2 && h.h[32w2].a + h.h[32w1].b + h.h[32w1].c > 32w20))) {
                                        tbl_controlhsindextest4l49.apply();
                                    } else if (hsiVar_1 == 32w2 && (hsiVar_0 == 32w1 && (hsiVar == 32w2 && h.h[32w2].a + h.h[32w1].b + h.h[32w2].c > 32w20))) {
                                        tbl_controlhsindextest4l49.apply();
                                    } else {
                                        tbl_controlhsindextest4l48_11.apply();
                                        if (hsiVar_1 == 32w0 && (hsiVar_0 == 32w2 && (hsiVar == 32w2 && h.h[32w2].a + h.h[32w2].b + h.h[32w0].c > 32w20))) {
                                            tbl_controlhsindextest4l49.apply();
                                        } else if (hsiVar_1 == 32w1 && (hsiVar_0 == 32w2 && (hsiVar == 32w2 && h.h[32w2].a + h.h[32w2].b + h.h[32w1].c > 32w20))) {
                                            tbl_controlhsindextest4l49.apply();
                                        } else if (hsiVar_1 == 32w2 && (hsiVar_0 == 32w2 && (hsiVar == 32w2 && h.h[32w2].a + h.h[32w2].b + h.h[32w2].c > 32w20))) {
                                            tbl_controlhsindextest4l49.apply();
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
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
