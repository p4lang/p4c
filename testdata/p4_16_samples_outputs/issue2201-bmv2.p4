#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

typedef bit<48> EthernetAddress;
header ethernet_t {
    EthernetAddress dstAddr;
    EthernetAddress srcAddr;
    bit<16>         etherType;
}

header ipv4_t {
    bit<4>  version;
    bit<4>  ihl;
    bit<8>  diffserv;
    bit<16> totalLen;
    bit<16> identification;
    bit<3>  flags;
    bit<13> fragOffset;
    bit<8>  ttl;
    bit<8>  protocol;
    bit<16> hdrChecksum;
    bit<32> srcAddr;
    bit<32> dstAddr;
}

struct headers_t {
    ethernet_t ethernet;
    ipv4_t     ipv4;
}

struct metadata_t {
}

parser parserImpl(packet_in packet, out headers_t hdr, inout metadata_t meta, inout standard_metadata_t stdmeta) {
    state start {
        packet.extract(hdr.ethernet);
        transition select(hdr.ethernet.etherType) {
            0x800: parse_ipv4;
            default: accept;
        }
    }
    state parse_ipv4 {
        packet.extract(hdr.ipv4);
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
        log_msg("GREPME Packet ingress begin");
        bool1 = (bool)hdr.ethernet.dstAddr[0:0];
        log_msg("GREPME bool1={}", { bool1 });
        log_msg("GREPME (bit<1>) bool1={}", { (bit<1>)bool1 });
        log_msg("GREPME (bit<1>) (!bool1)={}", { (bit<1>)!bool1 });
        bit1 = hdr.ethernet.dstAddr[0:0];
        log_msg("GREPME bit1={}", { bit1 });
        signed1 = 127;
        signed1 = signed1 + 1;
        log_msg("GREPME signed1={}", { signed1 });
        unsigned1 = 127;
        unsigned1 = unsigned1 + 1;
        log_msg("GREPME unsigned1={}", { unsigned1 });
        log_msg("GREPME hdr.ethernet.dstAddr[1:0]={}", { hdr.ethernet.dstAddr[1:0] });
        if (hdr.ethernet.dstAddr[1:0] == 0) {
            enum1 = MyEnum_t.VAL1;
        } else if (hdr.ethernet.dstAddr[1:0] == 1) {
            enum1 = MyEnum_t.VAL2;
        } else {
            enum1 = MyEnum_t.VAL3;
        }
        log_msg("GREPME enum1={}", { enum1 });
        if (hdr.ethernet.dstAddr[1:0] == 0) {
            serenum1 = MySerializableEnum_t.VAL17;
        } else if (hdr.ethernet.dstAddr[1:0] == 1) {
            serenum1 = MySerializableEnum_t.VAL23;
        } else {
            serenum1 = MySerializableEnum_t.VAL19;
        }
        log_msg("GREPME serenum1={}", { serenum1 });
        log_msg("GREPME hdr.ethernet.isValid()={}", { hdr.ethernet.isValid() });
        log_msg("GREPME hdr.ethernet={}", { hdr.ethernet });
        log_msg("GREPME hdr.ipv4.isValid()={}", { hdr.ipv4.isValid() });
        log_msg("GREPME hdr.ipv4={}", { hdr.ipv4 });
        log_msg("GREPME stdmeta={}", { stdmeta });
        log_msg("GREPME error.PacketTooShort={}", { error.PacketTooShort });
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
