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
    bit<4> val1;
    int<3> val2;
    bool   val3;
}

struct Structure1 {
    bit<8> val1;
    bit<8> val2;
    bool   val3;
}

struct Structure2 {
    int<4> val1;
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
        packet.extract<ethernet_t>(hdr.ethernet);
        transition accept;
    }
}

control verifyChecksum(inout headers_t hdr, inout metadata_t meta) {
    apply {
    }
}

control ingressImpl(inout headers_t hdr, inout metadata_t meta, inout standard_metadata_t stdmeta) {
    apply {
        Header hdr1 = (Header){val1 = 4w3,val2 = -3s1,val3 = true};
        log_msg<tuple<Header>>("hdr1 = {}", { hdr1 });
        log_msg<tuple<tuple<bit<4>, int<4>, Header>>>("list: {}", { { 4w2, 4s3, hdr1 } });
        tuple<bit<32>, bool, int<32>> x = { 32w10, true, 32s7 };
        log_msg<tuple<tuple<bit<32>, bool, int<32>>>>("tuple x = {}", { x });
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

