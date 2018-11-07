#include <core.p4>
#include <v1model.p4>

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
    bit<16> tmp;
    bit<16> tmp_0;
    bit<16> x_0;
    bool hasReturned;
    bit<16> retval;
    bit<16> x_1;
    bool hasReturned_0;
    bit<16> retval_0;
    state start {
        x_0 = (bit<16>)standard_metadata.ingress_port;
        hasReturned = false;
        hasReturned = true;
        retval = x_0 + 16w1;
        tmp = retval;
        tmp_port_0 = tmp;
        packet.extract<ethernet_t>(hdr.ethernet);
        x_1 = hdr.ethernet.etherType;
        hasReturned_0 = false;
        hasReturned_0 = true;
        retval_0 = x_1 + 16w1;
        tmp_0 = retval_0;
        hdr.ethernet.etherType = tmp_0;
        meta.tmp_port = tmp_port_0;
        transition accept;
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
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
        hdr.ethernet.srcAddr[15:0] = hdr.ethernet.srcAddr[15:0] + (hdr.ethernet.srcAddr[15:0] + 16w1);
        hdr.ethernet.srcAddr[15:0] = hdr.ethernet.srcAddr[15:0] + 16w1;
        hdr.ethernet.etherType = hdr.ethernet.etherType + 16w1;
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

