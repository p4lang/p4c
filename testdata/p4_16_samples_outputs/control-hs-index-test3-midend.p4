#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

header ethernet_t {
    bit<48> dst_addr;
    bit<48> src_addr;
    bit<16> eth_type;
    bit<32> index;
}

header H {
    bit<32> a;
}

struct headers {
    ethernet_t[3] eth_hdr;
    H             h;
}

struct Meta {
}

parser p(packet_in pkt, out headers hdr, inout Meta m, inout standard_metadata_t sm) {
    state start {
        pkt.extract<ethernet_t>(hdr.eth_hdr[0]);
        pkt.extract<ethernet_t>(hdr.eth_hdr[1]);
        pkt.extract<ethernet_t>(hdr.eth_hdr[2]);
        transition accept;
    }
}

control ingress(inout headers h, inout Meta m, inout standard_metadata_t sm) {
    @hidden action controlhsindextest3l38() {
        if (h.h.a == 32w0) {
            if (h.eth_hdr[0].index == 32w0) {
                h.eth_hdr[0].index = 32w1;
            } else if (h.eth_hdr[0].index == 32w1) {
                h.eth_hdr[1].index = 32w1;
            } else if (h.eth_hdr[0].index == 32w2) {
                h.eth_hdr[2].index = 32w1;
            }
        } else if (h.h.a == 32w1) {
            if (h.eth_hdr[1].index == 32w0) {
                h.eth_hdr[0].index = 32w1;
            } else if (h.eth_hdr[1].index == 32w1) {
                h.eth_hdr[1].index = 32w1;
            } else if (h.eth_hdr[1].index == 32w2) {
                h.eth_hdr[2].index = 32w1;
            }
        } else if (h.h.a == 32w2) {
            if (h.eth_hdr[2].index == 32w0) {
                h.eth_hdr[0].index = 32w1;
            } else if (h.eth_hdr[2].index == 32w1) {
                h.eth_hdr[1].index = 32w1;
            } else if (h.eth_hdr[2].index == 32w2) {
                h.eth_hdr[2].index = 32w1;
            }
        }
    }
    @hidden action controlhsindextest3l39() {
        if (h.h.a == 32w0) {
            h.eth_hdr[0].setInvalid();
        } else if (h.h.a == 32w1) {
            h.eth_hdr[1].setInvalid();
        } else if (h.h.a == 32w2) {
            h.eth_hdr[2].setInvalid();
        }
    }
    @hidden table tbl_controlhsindextest3l38 {
        actions = {
            controlhsindextest3l38();
        }
        const default_action = controlhsindextest3l38();
    }
    @hidden table tbl_controlhsindextest3l39 {
        actions = {
            controlhsindextest3l39();
        }
        const default_action = controlhsindextest3l39();
    }
    apply {
        if (h.h.a == 32w0 && h.eth_hdr[0].isValid()) {
            tbl_controlhsindextest3l38.apply();
        } else if (h.h.a == 32w1 && h.eth_hdr[1].isValid()) {
            tbl_controlhsindextest3l38.apply();
        } else if (h.h.a == 32w2 && h.eth_hdr[2].isValid()) {
            tbl_controlhsindextest3l38.apply();
        }
        tbl_controlhsindextest3l39.apply();
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
        pkt.emit<ethernet_t>(h.eth_hdr[0]);
    }
}

V1Switch<headers, Meta>(p(), vrfy(), ingress(), egress(), update(), deparser()) main;

