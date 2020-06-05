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

control ingressImpl(inout headers_t hdr, inout metadata_t meta, inout standard_metadata_t stdmeta) {
    @name("ingressImpl.act_miss") action act_miss() {
        hdr.ethernet.srcAddr = 48w0xdeadbeef;
    }
    @name("ingressImpl.act_hit") action act_hit(bit<48> x) {
        hdr.ethernet.srcAddr = x;
    }
    @name("ingressImpl.lpm1") table lpm1_0 {
        key = {
            hdr.ethernet.dstAddr: lpm @name("hdr.ethernet.dstAddr") ;
        }
        actions = {
            act_miss();
            act_hit();
        }
        const entries = {
                        48w0xa0102030000 &&& 48w0xffffffffffff : act_hit(48w1);
                        48w0xa0000000000 &&& 48w0xff0000000000 : act_hit(48w2);
                        48w0x0 &&& 48w0x0 : act_hit(48w3);
        }

        const default_action = act_miss();
    }
    @hidden action v1modelconstentriesbmv2l77() {
        stdmeta.egress_spec = 9w1;
    }
    @hidden table tbl_v1modelconstentriesbmv2l77 {
        actions = {
            v1modelconstentriesbmv2l77();
        }
        const default_action = v1modelconstentriesbmv2l77();
    }
    apply {
        lpm1_0.apply();
        tbl_v1modelconstentriesbmv2l77.apply();
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

