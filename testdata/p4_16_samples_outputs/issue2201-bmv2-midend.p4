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
    bit<48> f0;
    bit<48> f1;
    bit<16> f2;
}

struct tuple_8 {
    bit<4>  f0;
    bit<4>  f1;
    bit<8>  f2;
    bit<16> f3;
    bit<16> f4;
    bit<3>  f5;
    bit<13> f6;
    bit<8>  f7;
    bit<8>  f8;
    bit<16> f9;
    bit<32> f10;
    bit<32> f11;
}

struct tuple_9 {
    bit<9>  f0;
    bit<9>  f1;
    bit<9>  f2;
    bit<32> f3;
    bit<32> f4;
    bit<32> f5;
    bit<19> f6;
    bit<32> f7;
    bit<19> f8;
    bit<48> f9;
    bit<48> f10;
    bit<16> f11;
    bit<16> f12;
    bit<1>  f13;
    error   f14;
    bit<3>  f15;
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
        log_msg<tuple_7>("GREPME hdr.ethernet=(dstAddr:{},srcAddr:{},etherType:{})", (tuple_7){f0 = hdr.ethernet.dstAddr,f1 = hdr.ethernet.srcAddr,f2 = hdr.ethernet.etherType});
        log_msg<tuple_0>("GREPME hdr.ipv4.isValid()={}", (tuple_0){f0 = hdr.ipv4.isValid()});
        log_msg<tuple_8>("GREPME hdr.ipv4=(version:{},ihl:{},diffserv:{},totalLen:{},identification:{},flags:{},fragOffset:{},ttl:{},protocol:{},hdrChecksum:{},srcAddr:{},dstAddr:{})", (tuple_8){f0 = hdr.ipv4.version,f1 = hdr.ipv4.ihl,f2 = hdr.ipv4.diffserv,f3 = hdr.ipv4.totalLen,f4 = hdr.ipv4.identification,f5 = hdr.ipv4.flags,f6 = hdr.ipv4.fragOffset,f7 = hdr.ipv4.ttl,f8 = hdr.ipv4.protocol,f9 = hdr.ipv4.hdrChecksum,f10 = hdr.ipv4.srcAddr,f11 = hdr.ipv4.dstAddr});
        log_msg<tuple_9>("GREPME stdmeta=(ingress_port:{},egress_spec:{},egress_port:{},instance_type:{},packet_length:{},enq_timestamp:{},enq_qdepth:{},deq_timedelta:{},deq_qdepth:{},ingress_global_timestamp:{},egress_global_timestamp:{},mcast_grp:{},egress_rid:{},checksum_error:{},parser_error:{},priority:{})", (tuple_9){f0 = stdmeta.ingress_port,f1 = stdmeta.egress_spec,f2 = stdmeta.egress_port,f3 = stdmeta.instance_type,f4 = stdmeta.packet_length,f5 = stdmeta.enq_timestamp,f6 = stdmeta.enq_qdepth,f7 = stdmeta.deq_timedelta,f8 = stdmeta.deq_qdepth,f9 = stdmeta.ingress_global_timestamp,f10 = stdmeta.egress_global_timestamp,f11 = stdmeta.mcast_grp,f12 = stdmeta.egress_rid,f13 = stdmeta.checksum_error,f14 = stdmeta.parser_error,f15 = stdmeta.priority});
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
