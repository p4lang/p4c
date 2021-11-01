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
    @name("ingress.tmp_0") bit<1> tmp;
    @name("ingress.tmp_1") bit<1> tmp_0;
    @name("ingress.val_0") bit<1> val;
    @name("ingress.bound_0") bit<1> bound;
    @name("ingress.hasReturned") bool hasReturned;
    @name("ingress.retval") bit<1> retval;
    @name("ingress.tmp") bit<1> tmp_1;
    bit<1> hsiVar;
    @name("ingress.perform_action") action perform_action() {
        val = (bool_val_0 ? 1w0 : val);
        bound = (bool_val_0 ? 1w1 : bound);
        hasReturned = (bool_val_0 ? false : hasReturned);
        tmp_1 = (bool_val_0 ? (val < bound ? val : tmp_1) : tmp_1);
        tmp_1 = (bool_val_0 ? (val < bound ? val : bound) : tmp_1);
        hasReturned = (bool_val_0 ? true : hasReturned);
        retval = (bool_val_0 ? tmp_1 : retval);
        tmp = (bool_val_0 ? retval : tmp);
        tmp_0 = (bool_val_0 ? tmp : tmp_0);
        hsiVar = (bool_val_0 ? tmp_0 : 1w0);
        if (hsiVar == 1w0) {
            h.h[1w0].a = (bool_val_0 ? 8w1 : h.h[1w0].a);
        } else if (hsiVar == 1w1) {
            h.h[1w1].a = (bool_val_0 ? 8w1 : h.h[1w1].a);
        } else if (hsiVar == 1w0) {
            h.h[1w0].a = (bool_val_0 ? 8w1 : h.h[1w0].a);
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

