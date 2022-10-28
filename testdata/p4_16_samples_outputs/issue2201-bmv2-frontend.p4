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

enum MyEnum_t {
    VAL1,
    VAL2,
    VAL3
}

enum bit<10> MySerializableEnum_t {
    VAL17 = 10w17,
    VAL23 = 10w23,
    VAL19 = 10w19
}

control ingressImpl(inout headers_t hdr, inout metadata_t meta, inout standard_metadata_t stdmeta) {
    @name("ingressImpl.bool1") bool bool1_0;
    @name("ingressImpl.bit1") bit<1> bit1_0;
    @name("ingressImpl.enum1") MyEnum_t enum1_0;
    @name("ingressImpl.serenum1") MySerializableEnum_t serenum1_0;
    @name("ingressImpl.signed1") int<8> signed1_0;
    @name("ingressImpl.unsigned1") bit<8> unsigned1_0;
    apply {
        bool1_0 = (bool)hdr.ethernet.dstAddr[0:0];
        log_msg<tuple<bool>>("bool1={}", { bool1_0 });
        log_msg<tuple<bit<1>>>("(bit<1>) bool1={}", { (bit<1>)bool1_0 });
        log_msg<tuple<bit<1>>>("(bit<1>) (!bool1)={}", { (bit<1>)!bool1_0 });
        bit1_0 = hdr.ethernet.dstAddr[0:0];
        log_msg<tuple<bit<1>>>("bit1={}", { bit1_0 });
        signed1_0 = 8s127;
        signed1_0 = signed1_0 + 8s1;
        log_msg<tuple<int<8>>>("signed1={}", { signed1_0 });
        unsigned1_0 = 8w127;
        unsigned1_0 = unsigned1_0 + 8w1;
        log_msg<tuple<bit<8>>>("unsigned1={}", { unsigned1_0 });
        if (hdr.ethernet.dstAddr[1:0] == 2w0) {
            enum1_0 = MyEnum_t.VAL1;
        } else if (hdr.ethernet.dstAddr[1:0] == 2w1) {
            enum1_0 = MyEnum_t.VAL2;
        } else {
            enum1_0 = MyEnum_t.VAL3;
        }
        log_msg<tuple<MyEnum_t>>("enum1={}", { enum1_0 });
        if (hdr.ethernet.dstAddr[1:0] == 2w0) {
            serenum1_0 = MySerializableEnum_t.VAL17;
        } else if (hdr.ethernet.dstAddr[1:0] == 2w1) {
            serenum1_0 = MySerializableEnum_t.VAL23;
        } else {
            serenum1_0 = MySerializableEnum_t.VAL19;
        }
        log_msg<tuple<MySerializableEnum_t>>("serenum1={}", { serenum1_0 });
        log_msg<tuple<ethernet_t>>("hdr.ethernet={}", { hdr.ethernet });
        log_msg<tuple<standard_metadata_t>>("stdmeta={}", { stdmeta });
        log_msg<tuple<error>>("error.PacketTooShort={}", { error.PacketTooShort });
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
