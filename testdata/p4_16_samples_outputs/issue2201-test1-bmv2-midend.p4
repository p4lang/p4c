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

struct tuple_0 {
    bit<32> f0;
    bool    f1;
    int<32> f2;
}

struct tuple_1 {
    Header f0;
}

struct tuple_2 {
    bit<4> f0;
    int<4> f1;
    Header f2;
}

struct tuple_3 {
    tuple_2 f0;
}

struct tuple_4 {
    tuple_0 f0;
}

control ingressImpl(inout headers_t hdr, inout metadata_t meta, inout standard_metadata_t stdmeta) {
    @name("ingressImpl.hdr1") Header hdr1_0;
    @name("ingressImpl.x") tuple_0 x_0;
    @hidden action issue2201test1bmv2l83() {
        hdr1_0.setValid();
        hdr1_0.val1 = 4w3;
        hdr1_0.val2 = -3s1;
        hdr1_0.val3 = true;
        log_msg<tuple_1>("hdr1 = {}", { hdr1_0 });
        log_msg<tuple_3>("list: {}", { { 4w2, 4s3, hdr1_0 } });
        x_0.f0 = 32w10;
        x_0.f1 = true;
        x_0.f2 = 32s7;
        log_msg<tuple_4>("tuple x = {}", { x_0 });
    }
    @hidden table tbl_issue2201test1bmv2l83 {
        actions = {
            issue2201test1bmv2l83();
        }
        const default_action = issue2201test1bmv2l83();
    }
    apply {
        tbl_issue2201test1bmv2l83.apply();
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

