#include <core.p4>
#include <v1model.p4>

typedef bit<48> EthernetAddress;
header ethernet_t {
    EthernetAddress dstAddr;
    EthernetAddress srcAddr;
    bit<16>         etherType;
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
    fwd_meta_t fwd_meta;
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
    @name(".NoAction") action NoAction_0() {
    }
    bit<16> tmp_0;
    bit<32> x1_0;
    bit<16> x2_0;
    @name("ingress.debug_table_cksum1") table debug_table_cksum1_0 {
        key = {
            hdr.ethernet.srcAddr            : exact @name("hdr.ethernet.srcAddr") ;
            hdr.ethernet.dstAddr            : exact @name("hdr.ethernet.dstAddr") ;
            hdr.ethernet.etherType          : exact @name("hdr.ethernet.etherType") ;
            user_meta.fwd_meta.exp_etherType: exact @name("user_meta.fwd_meta.exp_etherType") ;
            user_meta.fwd_meta.tmp          : exact @name("user_meta.fwd_meta.tmp") ;
            user_meta.fwd_meta.exp_x1       : exact @name("user_meta.fwd_meta.exp_x1") ;
            user_meta.fwd_meta.x1           : exact @name("user_meta.fwd_meta.x1") ;
            user_meta.fwd_meta.exp_x2       : exact @name("user_meta.fwd_meta.exp_x2") ;
            user_meta.fwd_meta.x2           : exact @name("user_meta.fwd_meta.x2") ;
            user_meta.fwd_meta.exp_x3       : exact @name("user_meta.fwd_meta.exp_x3") ;
            user_meta.fwd_meta.x3           : exact @name("user_meta.fwd_meta.x3") ;
            user_meta.fwd_meta.exp_x4       : exact @name("user_meta.fwd_meta.exp_x4") ;
            user_meta.fwd_meta.x4           : exact @name("user_meta.fwd_meta.x4") ;
        }
        actions = {
            NoAction_0();
        }
        default_action = NoAction_0();
    }
    apply {
        tmp_0 = ~hdr.ethernet.etherType;
        x1_0 = (bit<32>)tmp_0;
        x2_0 = x1_0[31:16] + x1_0[15:0];
        user_meta.fwd_meta.tmp = tmp_0;
        user_meta.fwd_meta.x1 = x1_0;
        user_meta.fwd_meta.x2 = x2_0;
        user_meta.fwd_meta.x3 = (bit<32>)~hdr.ethernet.etherType;
        user_meta.fwd_meta.x4 = ~(bit<32>)hdr.ethernet.etherType;
        user_meta.fwd_meta.exp_etherType = 16w0x800;
        user_meta.fwd_meta.exp_x1 = 32w0xf7ff;
        user_meta.fwd_meta.exp_x2 = 16w0xf7ff;
        user_meta.fwd_meta.exp_x3 = 32w0xf7ff;
        user_meta.fwd_meta.exp_x4 = 32w0xfffff7ff;
        hdr.ethernet.dstAddr = 48w0;
        if (hdr.ethernet.etherType != user_meta.fwd_meta.exp_etherType) 
            hdr.ethernet.dstAddr[47:40] = 8w1;
        if (user_meta.fwd_meta.x1 != user_meta.fwd_meta.exp_x1) 
            hdr.ethernet.dstAddr[39:32] = 8w1;
        if (user_meta.fwd_meta.x2 != user_meta.fwd_meta.exp_x2) 
            hdr.ethernet.dstAddr[31:24] = 8w1;
        if (user_meta.fwd_meta.x3 != user_meta.fwd_meta.exp_x3) 
            hdr.ethernet.dstAddr[23:16] = 8w1;
        if (user_meta.fwd_meta.x4 != user_meta.fwd_meta.exp_x4) 
            hdr.ethernet.dstAddr[15:8] = 8w1;
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

