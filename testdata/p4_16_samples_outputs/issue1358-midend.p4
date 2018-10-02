#include <core.p4>
#include <v1model.p4>

bit<16> incr(in bit<16> x) {
    return x + 16w1;
}
bit<16> twoxplus1(in bit<16> x) {
    return x + (x + 16w1);
}
struct metadata {
    bit<16> tmp_port;
}

header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
}

struct headers {
    ethernet_t ethernet;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    bit<16> tmp_port_0;
    bit<16> tmp_1;
    bit<16> tmp_2;
    bit<16> x_1;
    bool hasReturned_4;
    bit<16> retval_4;
    bit<16> x_2;
    bool hasReturned_5;
    bit<16> retval_5;
    state start {
        x_1 = (bit<16>)standard_metadata.ingress_port;
        hasReturned_4 = false;
        hasReturned_4 = true;
        retval_4 = x_1 + 16w1;
        tmp_1 = retval_4;
        tmp_port_0 = tmp_1;
        packet.extract<ethernet_t>(hdr.ethernet);
        x_2 = hdr.ethernet.etherType;
        hasReturned_5 = false;
        hasReturned_5 = true;
        retval_5 = x_2 + 16w1;
        tmp_2 = retval_5;
        hdr.ethernet.etherType = tmp_2;
        meta.tmp_port = tmp_port_0;
        transition accept;
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    bit<16> tmp_3;
    bit<16> tmp_4;
    bit<16> tmp_5;
    @name(".my_drop") action my_drop() {
        mark_to_drop();
    }
    @name("ingress.set_port") action set_port(bit<9> output_port) {
        standard_metadata.egress_spec = output_port;
    }
    @name("ingress.mac_da") table mac_da_0 {
        key = {
            hdr.ethernet.dstAddr: exact @name("hdr.ethernet.dstAddr") ;
        }
        actions = {
            set_port();
            my_drop();
        }
        default_action = my_drop();
    }
    @hidden action act() {
        tmp_3 = twoxplus1(hdr.ethernet.srcAddr[15:0]);
        hdr.ethernet.srcAddr[15:0] = tmp_3;
        tmp_4 = incr(hdr.ethernet.srcAddr[15:0]);
        hdr.ethernet.srcAddr[15:0] = tmp_4;
        tmp_5 = incr(hdr.ethernet.etherType);
        hdr.ethernet.etherType = tmp_5;
    }
    @hidden table tbl_act {
        actions = {
            act();
        }
        const default_action = act();
    }
    apply {
        mac_da_0.apply();
        tbl_act.apply();
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
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

V1Switch<headers, metadata>(ParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;

