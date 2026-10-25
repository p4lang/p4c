#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
}

header H {
    bit<8> x;
}

struct headers_t {
    ethernet_t ethernet;
    H          h1;
    H          u1_h;
}

struct metadata_t {
}

parser parserImpl(packet_in packet, out headers_t hdr, inout metadata_t meta, inout standard_metadata_t stdmeta) {
    state start {
        packet.extract<ethernet_t>(hdr.ethernet);
        transition accept;
    }
}

control verifyChecksum(inout headers_t hdr, inout metadata_t meta) {
    apply {
    }
}

control ingressImpl(inout headers_t hdr, inout metadata_t meta, inout standard_metadata_t stdmeta) {
    @name("ingressImpl.tmp") H tmp;
    H tmp_1_h;
    @name("ingressImpl.tmp_3") H[1] tmp_3;
    @name("ingressImpl.h") H h_0;
    H u_0_h;
    @name("ingressImpl.s") H[1] s_0;
    @hidden action issue30011l75() {
        log_msg("hdr.ethernet is valid");
    }
    @hidden action issue30011l77() {
        log_msg("hdr.ethernet is invalid");
    }
    @hidden action issue30011l90() {
        log_msg("f() returned a valid header");
    }
    @hidden action issue30011l92() {
        log_msg("f() returned an invalid header");
    }
    @hidden action issue30011l31() {
        h_0.setInvalid();
        tmp = h_0;
    }
    @hidden action issue30011l95() {
        log_msg("g() returned a header_union with valid member h");
    }
    @hidden action issue30011l97() {
        log_msg("g() returned a header_union with invalid member h");
    }
    @hidden action issue30011l36() {
        u_0_h.setInvalid();
        if (u_0_h.isValid()) {
            tmp_1_h.setValid();
            tmp_1_h = u_0_h;
        } else {
            tmp_1_h.setInvalid();
        }
    }
    @hidden action issue30011l100() {
        log_msg("h() returned a header stack with valid element 0");
    }
    @hidden action issue30011l102() {
        log_msg("h() returned a header stack with invalid element 0");
    }
    @hidden action issue30011l41() {
        s_0[0].setInvalid();
        tmp_3[0] = s_0[0];
    }
    @hidden table tbl_issue30011l75 {
        actions = {
            issue30011l75();
        }
        const default_action = issue30011l75();
    }
    @hidden table tbl_issue30011l77 {
        actions = {
            issue30011l77();
        }
        const default_action = issue30011l77();
    }
    @hidden table tbl_issue30011l31 {
        actions = {
            issue30011l31();
        }
        const default_action = issue30011l31();
    }
    @hidden table tbl_issue30011l90 {
        actions = {
            issue30011l90();
        }
        const default_action = issue30011l90();
    }
    @hidden table tbl_issue30011l92 {
        actions = {
            issue30011l92();
        }
        const default_action = issue30011l92();
    }
    @hidden table tbl_issue30011l36 {
        actions = {
            issue30011l36();
        }
        const default_action = issue30011l36();
    }
    @hidden table tbl_issue30011l95 {
        actions = {
            issue30011l95();
        }
        const default_action = issue30011l95();
    }
    @hidden table tbl_issue30011l97 {
        actions = {
            issue30011l97();
        }
        const default_action = issue30011l97();
    }
    @hidden table tbl_issue30011l41 {
        actions = {
            issue30011l41();
        }
        const default_action = issue30011l41();
    }
    @hidden table tbl_issue30011l100 {
        actions = {
            issue30011l100();
        }
        const default_action = issue30011l100();
    }
    @hidden table tbl_issue30011l102 {
        actions = {
            issue30011l102();
        }
        const default_action = issue30011l102();
    }
    apply {
        if (hdr.ethernet.isValid()) {
            tbl_issue30011l75.apply();
        } else {
            tbl_issue30011l77.apply();
        }
        tbl_issue30011l31.apply();
        if (tmp.isValid()) {
            tbl_issue30011l90.apply();
        } else {
            tbl_issue30011l92.apply();
        }
        tbl_issue30011l36.apply();
        if (tmp_1_h.isValid()) {
            tbl_issue30011l95.apply();
        } else {
            tbl_issue30011l97.apply();
        }
        tbl_issue30011l41.apply();
        if (tmp_3[0].isValid()) {
            tbl_issue30011l100.apply();
        } else {
            tbl_issue30011l102.apply();
        }
    }
}

control egressImpl(inout headers_t hdr, inout metadata_t meta, inout standard_metadata_t stdmeta) {
    apply {
    }
}

control updateChecksum(inout headers_t hdr, inout metadata_t meta) {
    apply {
    }
}

control deparserImpl(packet_out packet, in headers_t hdr) {
    apply {
        packet.emit<ethernet_t>(hdr.ethernet);
    }
}

V1Switch<headers_t, metadata_t>(parserImpl(), verifyChecksum(), ingressImpl(), egressImpl(), updateChecksum(), deparserImpl()) main;
