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

H f() {
    H h;
    return h;
}
U g() {
    U u;
    return u;
}
H[1] h() {
    H[1] s;
    return s;
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
        packet.extract(hdr.ethernet);
        transition accept;
    }
}

control verifyChecksum(inout headers_t hdr, inout metadata_t meta) {
    apply {
    }
}

control ingressImpl(inout headers_t hdr, inout metadata_t meta, inout standard_metadata_t stdmeta) {
    apply {
        if (hdr.ethernet.isValid()) {
            log_msg("hdr.ethernet is valid");
        } else {
            log_msg("hdr.ethernet is invalid");
        }
        if (f().isValid()) {
            log_msg("f() returned a valid header");
        } else {
            log_msg("f() returned an invalid header");
        }
        if (g().h.isValid()) {
            log_msg("g() returned a header_union with valid member h");
        } else {
            log_msg("g() returned a header_union with invalid member h");
        }
        if (h()[0].isValid()) {
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
        packet.emit(hdr.ethernet);
    }
}

V1Switch(parserImpl(), verifyChecksum(), ingressImpl(), egressImpl(), updateChecksum(), deparserImpl()) main;
