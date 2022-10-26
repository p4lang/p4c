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
        packet.extract(hdr.ethernet);
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
    VAL17 = 17,
    VAL23 = 23,
    VAL19 = 19
}

control ingressImpl(inout headers_t hdr, inout metadata_t meta, inout standard_metadata_t stdmeta) {
    bool bool1;
    bit<1> bit1;
    MyEnum_t enum1;
    MySerializableEnum_t serenum1;
    int<8> signed1;
    bit<8> unsigned1;
    apply {
        bool1 = (bool)hdr.ethernet.dstAddr[0:0];
        log_msg("bool1={}", { bool1 });
        log_msg("(bit<1>) bool1={}", { (bit<1>)bool1 });
        log_msg("(bit<1>) (!bool1)={}", { (bit<1>)!bool1 });
        bit1 = hdr.ethernet.dstAddr[0:0];
        log_msg("bit1={}", { bit1 });
        signed1 = 127;
        signed1 = signed1 + 1;
        log_msg("signed1={}", { signed1 });
        unsigned1 = 127;
        unsigned1 = unsigned1 + 1;
        log_msg("unsigned1={}", { unsigned1 });
        if (hdr.ethernet.dstAddr[1:0] == 0) {
            enum1 = MyEnum_t.VAL1;
        } else if (hdr.ethernet.dstAddr[1:0] == 1) {
            enum1 = MyEnum_t.VAL2;
        } else {
            enum1 = MyEnum_t.VAL3;
        }
        log_msg("enum1={}", { enum1 });
        if (hdr.ethernet.dstAddr[1:0] == 0) {
            serenum1 = MySerializableEnum_t.VAL17;
        } else if (hdr.ethernet.dstAddr[1:0] == 1) {
            serenum1 = MySerializableEnum_t.VAL23;
        } else {
            serenum1 = MySerializableEnum_t.VAL19;
        }
        log_msg("serenum1={}", { serenum1 });
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
