#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

typedef bit<48> EthernetAddress;
header ethernet_t {
    EthernetAddress dstAddr;
    EthernetAddress srcAddr;
    bit<16>         etherType;
}

struct headers_t {
    ethernet_t ethernet;
}

struct metadata_t {
}

header Header {
    bit<7> val1;
    bool   val2;
}

struct Structure1 {
    bit<8> val1;
    bit<8> val2;
    bool   val3;
}

struct Structure2 {
    int<7> val1;
    Header val2;
}

struct Structure3 {
    bool       val1;
    Header     val2;
    int<4>     val3;
    Header     val4;
    Structure1 val5;
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
        Structure1 str1 = { 2, 3, false };
        log_msg("str1 = {}", { str1 });
        Header hdr1 = { 4, true };
        Structure2 str2 = { 3, hdr1 };
        log_msg("str2 = {}", { str2 });
        Header hdr2 = { 3, false };
        Structure3 str3 = { true, hdr1, -2, hdr2, str1 };
        log_msg("str3 = {}", { str3 });
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

