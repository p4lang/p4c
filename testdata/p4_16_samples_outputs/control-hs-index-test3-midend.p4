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
        h.h[0].a = 32w1;
    }
    @hidden action controlhsindextest3l44_3() {
        h.h[1].a = 32w1;
    }
    @hidden action controlhsindextest3l44_4() {
        h.h[2].a = 32w1;
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
    @hidden action controlhsindextest3l45() {
        h.h[0].setInvalid();
    }
    @hidden action controlhsindextest3l45_0() {
        h.h[1].setInvalid();
    }
    @hidden action controlhsindextest3l45_1() {
        h.h[2].setInvalid();
    }
    @hidden table tbl_controlhsindextest3l44 {
        actions = {
            controlhsindextest3l44();
        }
        const default_action = controlhsindextest3l44();
    }
    @hidden table tbl_controlhsindextest3l44_0 {
        actions = {
            controlhsindextest3l44_0();
        }
        const default_action = controlhsindextest3l44_0();
    }
    @hidden table tbl_controlhsindextest3l44_1 {
        actions = {
            controlhsindextest3l44_1();
        }
        const default_action = controlhsindextest3l44_1();
    }
    @hidden table tbl_controlhsindextest3l44_2 {
        actions = {
            controlhsindextest3l44_2();
        }
        const default_action = controlhsindextest3l44_2();
    }
    @hidden table tbl_controlhsindextest3l44_3 {
        actions = {
            controlhsindextest3l44_3();
        }
        const default_action = controlhsindextest3l44_3();
    }
    @hidden table tbl_controlhsindextest3l44_4 {
        actions = {
            controlhsindextest3l44_4();
        }
        const default_action = controlhsindextest3l44_4();
    }
    @hidden table tbl_controlhsindextest3l44_5 {
        actions = {
            controlhsindextest3l44_5();
        }
        const default_action = controlhsindextest3l44_5();
    }
    @hidden table tbl_controlhsindextest3l44_6 {
        actions = {
            controlhsindextest3l44_6();
        }
        const default_action = controlhsindextest3l44_6();
    }
    @hidden table tbl_controlhsindextest3l44_7 {
        actions = {
            controlhsindextest3l44_7();
        }
        const default_action = controlhsindextest3l44_7();
    }
    @hidden table tbl_controlhsindextest3l45 {
        actions = {
            controlhsindextest3l45();
        }
        const default_action = controlhsindextest3l45();
    }
    @hidden table tbl_controlhsindextest3l45_0 {
        actions = {
            controlhsindextest3l45_0();
        }
        const default_action = controlhsindextest3l45_0();
    }
    @hidden table tbl_controlhsindextest3l45_1 {
        actions = {
            controlhsindextest3l45_1();
        }
        const default_action = controlhsindextest3l45_1();
    }
    apply {
        if (h.i.index == 32w0 && h.h[0].isValid()) {
            if (h.h[0].a == 32w0) {
                tbl_controlhsindextest3l44.apply();
            } else if (h.h[0].a == 32w1) {
                tbl_controlhsindextest3l44_0.apply();
            } else if (h.h[0].a == 32w2) {
                tbl_controlhsindextest3l44_1.apply();
            }
        } else if (h.i.index == 32w1 && h.h[1].isValid()) {
            if (h.h[1].a == 32w0) {
                tbl_controlhsindextest3l44_2.apply();
            } else if (h.h[1].a == 32w1) {
                tbl_controlhsindextest3l44_3.apply();
            } else if (h.h[1].a == 32w2) {
                tbl_controlhsindextest3l44_4.apply();
            }
        } else if (h.i.index == 32w2 && h.h[2].isValid()) {
            if (h.h[2].a == 32w0) {
                tbl_controlhsindextest3l44_5.apply();
            } else if (h.h[2].a == 32w1) {
                tbl_controlhsindextest3l44_6.apply();
            } else if (h.h[2].a == 32w2) {
                tbl_controlhsindextest3l44_7.apply();
            }
        }
        if (h.i.index == 32w0) {
            tbl_controlhsindextest3l45.apply();
        } else if (h.i.index == 32w1) {
            tbl_controlhsindextest3l45_0.apply();
        } else if (h.i.index == 32w2) {
            tbl_controlhsindextest3l45_1.apply();
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

