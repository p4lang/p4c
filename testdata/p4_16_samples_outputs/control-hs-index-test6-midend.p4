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
    bit<32> hsVar;
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @name("ingress.set_data") action set_data() {
    }
    bit<32> key_0;
    @name("ingress.t") table t_0 {
        actions = {
            set_data();
            @defaultonly NoAction_1();
        }
        key = {
            key_0: exact @name("h.h[h.i.index].a");
        }
        default_action = NoAction_1();
    }
    @hidden action controlhsindextest6l48() {
        key_0 = h.h[32w0].a;
    }
    @hidden action controlhsindextest6l48_0() {
        key_0 = h.h[32w1].a;
    }
    @hidden action controlhsindextest6l48_1() {
        key_0 = hsVar;
    }
    @hidden action controlhsindextest6l48_2() {
        hsiVar = h.i.index;
    }
    @hidden table tbl_controlhsindextest6l48 {
        actions = {
            controlhsindextest6l48_2();
        }
        const default_action = controlhsindextest6l48_2();
    }
    @hidden table tbl_controlhsindextest6l48_0 {
        actions = {
            controlhsindextest6l48();
        }
        const default_action = controlhsindextest6l48();
    }
    @hidden table tbl_controlhsindextest6l48_1 {
        actions = {
            controlhsindextest6l48_0();
        }
        const default_action = controlhsindextest6l48_0();
    }
    @hidden table tbl_controlhsindextest6l48_2 {
        actions = {
            controlhsindextest6l48_1();
        }
        const default_action = controlhsindextest6l48_1();
    }
    apply {
        tbl_controlhsindextest6l48.apply();
        if (hsiVar == 32w0) {
            tbl_controlhsindextest6l48_0.apply();
        } else if (hsiVar == 32w1) {
            tbl_controlhsindextest6l48_1.apply();
        } else if (hsiVar >= 32w1) {
            tbl_controlhsindextest6l48_2.apply();
        }
        t_0.apply();
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
