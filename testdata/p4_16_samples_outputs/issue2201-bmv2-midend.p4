#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
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
        packet.extract<ethernet_t>(hdr.ethernet);
        transition select(hdr.ethernet.etherType) {
            16w0x800: parse_ipv4;
            default: accept;
        }
    }
    state parse_ipv4 {
        packet.extract<ipv4_t>(hdr.ipv4);
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

struct tuple_0 {
    bool f0;
}

struct tuple_1 {
    bit<1> f0;
}

struct tuple_2 {
    int<8> f0;
}

struct tuple_3 {
    bit<8> f0;
}

struct tuple_4 {
    bit<2> f0;
}

struct tuple_5 {
    MyEnum_t f0;
}

struct tuple_6 {
    bit<10> f0;
}

struct tuple_7 {
    ethernet_t f0;
}

struct tuple_8 {
    ipv4_t f0;
}

struct tuple_9 {
    standard_metadata_t f0;
}

struct tuple_10 {
    error f0;
}

control ingressImpl(inout headers_t hdr, inout metadata_t meta, inout standard_metadata_t stdmeta) {
    @name("ingressImpl.enum1") MyEnum_t enum1_0;
    @name("ingressImpl.serenum1") bit<10> serenum1_0;
    @hidden action issue2201bmv2l139() {
        enum1_0 = MyEnum_t.VAL1;
    }
    @hidden action issue2201bmv2l141() {
        enum1_0 = MyEnum_t.VAL2;
    }
    @hidden action issue2201bmv2l143() {
        enum1_0 = MyEnum_t.VAL3;
    }
    @hidden action issue2201bmv2l101() {
        log_msg("GREPME Packet ingress begin");
        log_msg<tuple_0>("GREPME bool1={}", (tuple_0){f0 = (bool)hdr.ethernet.dstAddr[0:0]});
        log_msg<tuple_1>("GREPME (bit<1>) bool1={}", (tuple_1){f0 = (bit<1>)(bool)hdr.ethernet.dstAddr[0:0]});
        log_msg<tuple_1>("GREPME (bit<1>) (!bool1)={}", (tuple_1){f0 = (bit<1>)!(bool)hdr.ethernet.dstAddr[0:0]});
        log_msg<tuple_1>("GREPME bit1={}", (tuple_1){f0 = hdr.ethernet.dstAddr[0:0]});
        log_msg<tuple_2>("GREPME signed1={}", (tuple_2){f0 = -8s128});
        log_msg<tuple_3>("GREPME unsigned1={}", (tuple_3){f0 = 8w128});
        log_msg<tuple_4>("GREPME hdr.ethernet.dstAddr[1:0]={}", (tuple_4){f0 = hdr.ethernet.dstAddr[1:0]});
    }
    @hidden action issue2201bmv2l156() {
        serenum1_0 = 10w17;
    }
    @hidden action issue2201bmv2l158() {
        serenum1_0 = 10w23;
    }
    @hidden action issue2201bmv2l160() {
        serenum1_0 = 10w19;
    }
    @hidden action issue2201bmv2l152() {
        log_msg<tuple_5>("GREPME enum1={}", (tuple_5){f0 = enum1_0});
    }
    @hidden action issue2201bmv2l164() {
        log_msg<tuple_6>("GREPME serenum1={}", (tuple_6){f0 = serenum1_0});
        log_msg<tuple_0>("GREPME hdr.ethernet.isValid()={}", (tuple_0){f0 = hdr.ethernet.isValid()});
        log_msg<tuple_7>("GREPME hdr.ethernet={}", (tuple_7){f0 = hdr.ethernet});
        log_msg<tuple_0>("GREPME hdr.ipv4.isValid()={}", (tuple_0){f0 = hdr.ipv4.isValid()});
        log_msg<tuple_8>("GREPME hdr.ipv4={}", (tuple_8){f0 = hdr.ipv4});
        log_msg<tuple_9>("GREPME stdmeta={}", (tuple_9){f0 = stdmeta});
        log_msg<tuple_10>("GREPME error.PacketTooShort={}", (tuple_10){f0 = error.PacketTooShort});
    }
    @hidden table tbl_issue2201bmv2l101 {
        actions = {
            issue2201bmv2l101();
        }
        const default_action = issue2201bmv2l101();
    }
    @hidden table tbl_issue2201bmv2l139 {
        actions = {
            issue2201bmv2l139();
        }
        const default_action = issue2201bmv2l139();
    }
    @hidden table tbl_issue2201bmv2l141 {
        actions = {
            issue2201bmv2l141();
        }
        const default_action = issue2201bmv2l141();
    }
    @hidden table tbl_issue2201bmv2l143 {
        actions = {
            issue2201bmv2l143();
        }
        const default_action = issue2201bmv2l143();
    }
    @hidden table tbl_issue2201bmv2l152 {
        actions = {
            issue2201bmv2l152();
        }
        const default_action = issue2201bmv2l152();
    }
    @hidden table tbl_issue2201bmv2l156 {
        actions = {
            issue2201bmv2l156();
        }
        const default_action = issue2201bmv2l156();
    }
    @hidden table tbl_issue2201bmv2l158 {
        actions = {
            issue2201bmv2l158();
        }
        const default_action = issue2201bmv2l158();
    }
    @hidden table tbl_issue2201bmv2l160 {
        actions = {
            issue2201bmv2l160();
        }
        const default_action = issue2201bmv2l160();
    }
    @hidden table tbl_issue2201bmv2l164 {
        actions = {
            issue2201bmv2l164();
        }
        const default_action = issue2201bmv2l164();
    }
    apply {
        tbl_issue2201bmv2l101.apply();
        if (hdr.ethernet.dstAddr[1:0] == 2w0) {
            tbl_issue2201bmv2l139.apply();
        } else if (hdr.ethernet.dstAddr[1:0] == 2w1) {
            tbl_issue2201bmv2l141.apply();
        } else {
            tbl_issue2201bmv2l143.apply();
        }
        tbl_issue2201bmv2l152.apply();
        if (hdr.ethernet.dstAddr[1:0] == 2w0) {
            tbl_issue2201bmv2l156.apply();
        } else if (hdr.ethernet.dstAddr[1:0] == 2w1) {
            tbl_issue2201bmv2l158.apply();
        } else {
            tbl_issue2201bmv2l160.apply();
        }
        tbl_issue2201bmv2l164.apply();
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
