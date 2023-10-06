#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
}

struct fwd_meta_t {
    bit<16> tmp;
    bit<32> x1;
    bit<16> x2;
    bit<32> x3;
    bit<32> x4;
    bit<16> exp_etherType;
    bit<32> exp_x1;
    bit<16> exp_x2;
    bit<32> exp_x3;
    bit<32> exp_x4;
}

struct metadata {
    bit<16> _fwd_meta_tmp0;
    bit<32> _fwd_meta_x11;
    bit<16> _fwd_meta_x22;
    bit<32> _fwd_meta_x33;
    bit<32> _fwd_meta_x44;
    bit<16> _fwd_meta_exp_etherType5;
    bit<32> _fwd_meta_exp_x16;
    bit<16> _fwd_meta_exp_x27;
    bit<32> _fwd_meta_exp_x38;
    bit<32> _fwd_meta_exp_x49;
}

struct headers {
    ethernet_t ethernet;
}

parser IngressParserImpl(packet_in buffer, out headers hdr, inout metadata user_meta, inout standard_metadata_t standard_metadata) {
    state start {
        buffer.extract<ethernet_t>(hdr.ethernet);
        transition accept;
    }
}

control ingress(inout headers hdr, inout metadata user_meta, inout standard_metadata_t standard_metadata) {
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @name("ingress.debug_table_cksum1") table debug_table_cksum1_0 {
        key = {
            hdr.ethernet.srcAddr              : exact @name("hdr.ethernet.srcAddr");
            hdr.ethernet.dstAddr              : exact @name("hdr.ethernet.dstAddr");
            hdr.ethernet.etherType            : exact @name("hdr.ethernet.etherType");
            user_meta._fwd_meta_exp_etherType5: exact @name("user_meta.fwd_meta.exp_etherType");
            user_meta._fwd_meta_tmp0          : exact @name("user_meta.fwd_meta.tmp");
            user_meta._fwd_meta_exp_x16       : exact @name("user_meta.fwd_meta.exp_x1");
            user_meta._fwd_meta_x11           : exact @name("user_meta.fwd_meta.x1");
            user_meta._fwd_meta_exp_x27       : exact @name("user_meta.fwd_meta.exp_x2");
            user_meta._fwd_meta_x22           : exact @name("user_meta.fwd_meta.x2");
            user_meta._fwd_meta_exp_x38       : exact @name("user_meta.fwd_meta.exp_x3");
            user_meta._fwd_meta_x33           : exact @name("user_meta.fwd_meta.x3");
            user_meta._fwd_meta_exp_x49       : exact @name("user_meta.fwd_meta.exp_x4");
            user_meta._fwd_meta_x44           : exact @name("user_meta.fwd_meta.x4");
        }
        actions = {
            NoAction_1();
        }
        default_action = NoAction_1();
    }
    @hidden action issue983bmv2l108() {
        hdr.ethernet.dstAddr[47:40] = 8w1;
    }
    @hidden action issue983bmv2l92() {
        user_meta._fwd_meta_tmp0 = ~hdr.ethernet.etherType;
        user_meta._fwd_meta_x11 = (bit<32>)~hdr.ethernet.etherType;
        user_meta._fwd_meta_x22 = ((bit<32>)~hdr.ethernet.etherType)[31:16] + ~hdr.ethernet.etherType;
        user_meta._fwd_meta_x33 = (bit<32>)~hdr.ethernet.etherType;
        user_meta._fwd_meta_x44 = ~(bit<32>)hdr.ethernet.etherType;
        user_meta._fwd_meta_exp_etherType5 = 16w0x800;
        user_meta._fwd_meta_exp_x16 = 32w0xf7ff;
        user_meta._fwd_meta_exp_x27 = 16w0xf7ff;
        user_meta._fwd_meta_exp_x38 = 32w0xf7ff;
        user_meta._fwd_meta_exp_x49 = 32w0xfffff7ff;
        hdr.ethernet.dstAddr = 48w0;
    }
    @hidden action issue983bmv2l111() {
        hdr.ethernet.dstAddr[39:32] = 8w1;
    }
    @hidden action issue983bmv2l114() {
        hdr.ethernet.dstAddr[31:24] = 8w1;
    }
    @hidden action issue983bmv2l117() {
        hdr.ethernet.dstAddr[23:16] = 8w1;
    }
    @hidden action issue983bmv2l120() {
        hdr.ethernet.dstAddr[15:8] = 8w1;
    }
    @hidden table tbl_issue983bmv2l92 {
        actions = {
            issue983bmv2l92();
        }
        const default_action = issue983bmv2l92();
    }
    @hidden table tbl_issue983bmv2l108 {
        actions = {
            issue983bmv2l108();
        }
        const default_action = issue983bmv2l108();
    }
    @hidden table tbl_issue983bmv2l111 {
        actions = {
            issue983bmv2l111();
        }
        const default_action = issue983bmv2l111();
    }
    @hidden table tbl_issue983bmv2l114 {
        actions = {
            issue983bmv2l114();
        }
        const default_action = issue983bmv2l114();
    }
    @hidden table tbl_issue983bmv2l117 {
        actions = {
            issue983bmv2l117();
        }
        const default_action = issue983bmv2l117();
    }
    @hidden table tbl_issue983bmv2l120 {
        actions = {
            issue983bmv2l120();
        }
        const default_action = issue983bmv2l120();
    }
    apply {
        tbl_issue983bmv2l92.apply();
        if (hdr.ethernet.etherType != 16w0x800) {
            tbl_issue983bmv2l108.apply();
        }
        if ((bit<32>)~hdr.ethernet.etherType != 32w0xf7ff) {
            tbl_issue983bmv2l111.apply();
        }
        if (((bit<32>)~hdr.ethernet.etherType)[31:16] + ~hdr.ethernet.etherType != 16w0xf7ff) {
            tbl_issue983bmv2l114.apply();
        }
        if ((bit<32>)~hdr.ethernet.etherType != 32w0xf7ff) {
            tbl_issue983bmv2l117.apply();
        }
        if (~(bit<32>)hdr.ethernet.etherType != 32w0xfffff7ff) {
            tbl_issue983bmv2l120.apply();
        }
        debug_table_cksum1_0.apply();
    }
}

control egress(inout headers hdr, inout metadata user_meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        packet.emit<ethernet_t>(hdr.ethernet);
    }
}

control verifyChecksum(inout headers hdr, inout metadata meta) {
    apply {
    }
}

control computeChecksum(inout headers hdr, inout metadata meta) {
    apply {
    }
}

V1Switch<headers, metadata>(IngressParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;
