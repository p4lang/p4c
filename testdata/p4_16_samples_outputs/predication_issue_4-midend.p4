#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

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
    H[3]       h;
}

struct Meta {
}

parser p(packet_in pkt, out Headers hdr, inout Meta m, inout standard_metadata_t sm) {
    state start {
        pkt.extract<ethernet_t>(hdr.eth_hdr);
        pkt.extract<H>(hdr.h.next);
        pkt.extract<H>(hdr.h.next);
        pkt.extract<H>(hdr.h.next);
        transition accept;
    }
}

control ingress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    @name("ingress.bool_val") bool bool_val_0;
    @name("ingress.perform_action") action perform_action() {
        if (bool_val_0) {
            h.h[1w0].a = 8w1;
        }
    }
    @hidden action predication_issue_4l39() {
        bool_val_0 = h.eth_hdr.eth_type == 16w0xde;
    }
    @hidden table tbl_predication_issue_4l39 {
        actions = {
            predication_issue_4l39();
        }
        const default_action = predication_issue_4l39();
    }
    @hidden table tbl_perform_action {
        actions = {
            perform_action();
        }
        const default_action = perform_action();
    }
    apply {
        tbl_predication_issue_4l39.apply();
        tbl_perform_action.apply();
    }
}

control vrfy(inout Headers h, inout Meta m) {
    apply {
    }
}

control update(inout Headers h, inout Meta m) {
    apply {
    }
}

control egress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    apply {
    }
}

control deparser(packet_out b, in Headers h) {
    apply {
        b.emit<ethernet_t>(h.eth_hdr);
        b.emit<H>(h.h[0]);
        b.emit<H>(h.h[1]);
        b.emit<H>(h.h[2]);
    }
}

V1Switch<Headers, Meta>(p(), vrfy(), ingress(), egress(), update(), deparser()) main;
