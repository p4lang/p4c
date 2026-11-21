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
    bit<32> hsiVar;
    @hidden action controlhsindextest2l45() {
        h.h[32w0].a = 32w1;
    }
    @hidden action controlhsindextest2l45_0() {
        h.h[32w1].a = 32w1;
    }
    @hidden action controlhsindextest2l45_1() {
        h.h[32w2].a = 32w1;
    }
    @hidden action controlhsindextest2l44() {
        hsiVar = h.i.index + 32w1;
    }
    @hidden table tbl_controlhsindextest2l44 {
        actions = {
            controlhsindextest2l44();
        }
        const default_action = controlhsindextest2l44();
    }
    @hidden table tbl_controlhsindextest2l45 {
        actions = {
            controlhsindextest2l45();
        }
        const default_action = controlhsindextest2l45();
    }
    @hidden table tbl_controlhsindextest2l45_0 {
        actions = {
            controlhsindextest2l45_0();
        }
        const default_action = controlhsindextest2l45_0();
    }
    @hidden table tbl_controlhsindextest2l45_1 {
        actions = {
            controlhsindextest2l45_1();
        }
        const default_action = controlhsindextest2l45_1();
    }
    apply {
        tbl_controlhsindextest2l44.apply();
        if (hsiVar == 32w0 && h.h[32w0].a > 32w10) {
            tbl_controlhsindextest2l45.apply();
        } else if (hsiVar == 32w1 && h.h[32w1].a > 32w10) {
            tbl_controlhsindextest2l45_0.apply();
        } else if (hsiVar == 32w2 && h.h[32w2].a > 32w10) {
            tbl_controlhsindextest2l45_1.apply();
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
