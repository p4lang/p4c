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
    @hidden action issue30011l84() {
        log_msg("hdr.ethernet is valid");
    }
    @hidden action issue30011l86() {
        log_msg("hdr.ethernet is invalid");
    }
    @hidden action issue30011l99() {
        log_msg("f() returned a valid header");
    }
    @hidden action issue30011l101() {
        log_msg("f() returned an invalid header");
    }
    @hidden action issue30011l40() {
        h_0.setInvalid();
        tmp = h_0;
    }
    @hidden action issue30011l104() {
        log_msg("g() returned a header_union with valid member h");
    }
    @hidden action issue30011l106() {
        log_msg("g() returned a header_union with invalid member h");
    }
    @hidden action issue30011l45() {
        u_0_h.setInvalid();
        if (u_0_h.isValid()) {
            tmp_1_h.setValid();
            tmp_1_h = u_0_h;
        } else {
            tmp_1_h.setInvalid();
        }
    }
    @hidden action issue30011l109() {
        log_msg("h() returned a header stack with valid element 0");
    }
    @hidden action issue30011l111() {
        log_msg("h() returned a header stack with invalid element 0");
    }
    @hidden action issue30011l50() {
        s_0[0].setInvalid();
        tmp_3[0] = s_0[0];
    }
    @hidden table tbl_issue30011l84 {
        actions = {
            issue30011l84();
        }
        const default_action = issue30011l84();
    }
    @hidden table tbl_issue30011l86 {
        actions = {
            issue30011l86();
        }
        const default_action = issue30011l86();
    }
    @hidden table tbl_issue30011l40 {
        actions = {
            issue30011l40();
        }
        const default_action = issue30011l40();
    }
    @hidden table tbl_issue30011l99 {
        actions = {
            issue30011l99();
        }
        const default_action = issue30011l99();
    }
    @hidden table tbl_issue30011l101 {
        actions = {
            issue30011l101();
        }
        const default_action = issue30011l101();
    }
    @hidden table tbl_issue30011l45 {
        actions = {
            issue30011l45();
        }
        const default_action = issue30011l45();
    }
    @hidden table tbl_issue30011l104 {
        actions = {
            issue30011l104();
        }
        const default_action = issue30011l104();
    }
    @hidden table tbl_issue30011l106 {
        actions = {
            issue30011l106();
        }
        const default_action = issue30011l106();
    }
    @hidden table tbl_issue30011l50 {
        actions = {
            issue30011l50();
        }
        const default_action = issue30011l50();
    }
    @hidden table tbl_issue30011l109 {
        actions = {
            issue30011l109();
        }
        const default_action = issue30011l109();
    }
    @hidden table tbl_issue30011l111 {
        actions = {
            issue30011l111();
        }
        const default_action = issue30011l111();
    }
    apply {
        if (hdr.ethernet.isValid()) {
            tbl_issue30011l84.apply();
        } else {
            tbl_issue30011l86.apply();
        }
        tbl_issue30011l40.apply();
        if (tmp.isValid()) {
            tbl_issue30011l99.apply();
        } else {
            tbl_issue30011l101.apply();
        }
        tbl_issue30011l45.apply();
        if (tmp_1_h.isValid()) {
            tbl_issue30011l104.apply();
        } else {
            tbl_issue30011l106.apply();
        }
        tbl_issue30011l50.apply();
        if (tmp_3[0].isValid()) {
            tbl_issue30011l109.apply();
        } else {
            tbl_issue30011l111.apply();
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
