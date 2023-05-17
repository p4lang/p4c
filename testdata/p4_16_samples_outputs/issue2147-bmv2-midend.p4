#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
}

header H {
    bit<8> a;
}

struct Parsed_packet {
    ethernet_t eth;
    H          h;
}

struct Metadata {
}

control deparser(packet_out packet, in Parsed_packet hdr) {
    apply {
        packet.emit<ethernet_t>(hdr.eth);
        packet.emit<H>(hdr.h);
    }
}

parser p(packet_in pkt, out Parsed_packet hdr, inout Metadata meta, inout standard_metadata_t stdmeta) {
    state start {
        pkt.extract<ethernet_t>(hdr.eth);
        pkt.extract<H>(hdr.h);
        transition accept;
    }
}

control ingress(inout Parsed_packet hdr, inout Metadata meta, inout standard_metadata_t stdmeta) {
    @name("ingress.tmp") bit<7> tmp_0;
    @name("ingress.do_action") action do_action() {
        hdr.h.a[0:0] = 1w0;
    }
    @hidden action issue2147bmv2l59() {
        tmp_0 = hdr.h.a[7:1];
    }
    @hidden action issue2147bmv2l61() {
        hdr.h.a[7:1] = tmp_0;
    }
    @hidden table tbl_issue2147bmv2l59 {
        actions = {
            issue2147bmv2l59();
        }
        const default_action = issue2147bmv2l59();
    }
    @hidden table tbl_do_action {
        actions = {
            do_action();
        }
        const default_action = do_action();
    }
    @hidden table tbl_issue2147bmv2l61 {
        actions = {
            issue2147bmv2l61();
        }
        const default_action = issue2147bmv2l61();
    }
    apply {
        tbl_issue2147bmv2l59.apply();
        tbl_do_action.apply();
        tbl_issue2147bmv2l61.apply();
    }
}

control egress(inout Parsed_packet hdr, inout Metadata meta, inout standard_metadata_t stdmeta) {
    apply {
    }
}

control vrfy(inout Parsed_packet hdr, inout Metadata meta) {
    apply {
    }
}

control update(inout Parsed_packet hdr, inout Metadata meta) {
    apply {
    }
}

V1Switch<Parsed_packet, Metadata>(p(), vrfy(), ingress(), egress(), update(), deparser()) main;
