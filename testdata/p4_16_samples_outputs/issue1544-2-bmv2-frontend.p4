#include <core.p4>
#include <v1model.p4>

struct metadata {
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
    state start {
        packet.extract<ethernet_t>(hdr.ethernet);
        transition accept;
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".my_drop") action my_drop() {
        mark_to_drop();
    }
    bit<16> tmp;
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
    apply {
        mac_da_0.apply();
        {
            bit<16> x_0 = hdr.ethernet.srcAddr[15:0];
            bool hasReturned = false;
            bit<16> retval;
            bit<16> tmp_0;
            tmp_0 = x_0;
            if (x_0 > 16w5) 
                tmp_0 = x_0 + 16w65535;
            hasReturned = true;
            retval = tmp_0;
            tmp = retval;
        }
        hdr.ethernet.srcAddr[15:0] = tmp;
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

