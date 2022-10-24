#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

typedef bit<48> EthernetAddress;
header ethernet_t {
    EthernetAddress dstAddr;
    EthernetAddress srcAddr;
    bit<16>         etherType;
}

header H {
    bit<8> x;
}

header_union U {
    H h;
}

struct headers_t {
    ethernet_t ethernet;
    H          h1;
    U          u1;
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
    @name("ingressImpl.tmp_0") bool tmp_0;
    @name("ingressImpl.tmp_1") U tmp_1;
    @name("ingressImpl.tmp_2") bool tmp_2;
    @name("ingressImpl.tmp_3") H[1] tmp_3;
    @name("ingressImpl.tmp_4") bool tmp_4;
    @name("ingressImpl.retval") H retval;
    @name("ingressImpl.h") H h_0;
    @name("ingressImpl.retval_0") U retval_0;
    @name("ingressImpl.u") U u_0;
    @name("ingressImpl.retval_1") H[1] retval_1;
    @name("ingressImpl.s") H[1] s_0;
    apply {
        if (hdr.ethernet.isValid()) {
            log_msg("hdr.ethernet is valid");
        } else {
            log_msg("hdr.ethernet is invalid");
        }
        h_0.setInvalid();
        retval = h_0;
        tmp = retval;
        tmp_0 = tmp.isValid();
        if (tmp_0) {
            log_msg("f() returned a valid header");
        } else {
            log_msg("f() returned an invalid header");
        }
        u_0.h.setInvalid();
        retval_0 = u_0;
        tmp_1 = retval_0;
        tmp_2 = tmp_1.h.isValid();
        if (tmp_2) {
            log_msg("g() returned a header_union with valid member h");
        } else {
            log_msg("g() returned a header_union with invalid member h");
        }
        s_0[0].setInvalid();
        retval_1 = s_0;
        tmp_3 = retval_1;
        tmp_4 = tmp_3[0].isValid();
        if (tmp_4) {
            log_msg("h() returned a header stack with valid element 0");
        } else {
            log_msg("h() returned a header stack with invalid element 0");
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
