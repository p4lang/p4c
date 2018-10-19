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
    state start {
        {
            bit<16> x_0 = (bit<16>)standard_metadata.ingress_port;
            bool hasReturned = false;
            bit<16> retval;
            hasReturned = true;
            retval = x_0 + 16w1;
            tmp = retval;
        }
        tmp_port_0 = tmp;
        packet.extract<ethernet_t>(hdr.ethernet);
        {
            bit<16> x_1 = hdr.ethernet.etherType;
            bool hasReturned_0 = false;
            bit<16> retval_0;
            hasReturned_0 = true;
            retval_0 = x_1 + 16w1;
            tmp_0 = retval_0;
        }
        hdr.ethernet.etherType = tmp_0;
        meta.tmp_port = tmp_port_0;
        transition accept;
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".my_drop") action my_drop() {
        mark_to_drop();
    }
    bit<16> tmp_1;
    bit<16> tmp_2;
    bit<16> tmp_3;
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
            bit<16> x_2 = hdr.ethernet.srcAddr[15:0];
            bool hasReturned_3 = false;
            bit<16> retval_3;
            bit<16> tmp_4;
            bit<16> tmp_5;
            {
                bit<16> x_3 = x_2;
                bool hasReturned_4 = false;
                bit<16> retval_4;
                hasReturned_4 = true;
                retval_4 = x_3 + 16w1;
                tmp_4 = retval_4;
            }
            tmp_5 = x_2 + tmp_4;
            hasReturned_3 = true;
            retval_3 = tmp_5;
            tmp_1 = retval_3;
        }
        hdr.ethernet.srcAddr[15:0] = tmp_1;
        {
            bit<16> x_4 = hdr.ethernet.srcAddr[15:0];
            bool hasReturned_5 = false;
            bit<16> retval_5;
            hasReturned_5 = true;
            retval_5 = x_4 + 16w1;
            tmp_2 = retval_5;
        }
        hdr.ethernet.srcAddr[15:0] = tmp_2;
        {
            bit<16> x_5 = hdr.ethernet.etherType;
            bool hasReturned_6 = false;
            bit<16> retval_6;
            hasReturned_6 = true;
            retval_6 = x_5 + 16w1;
            tmp_3 = retval_6;
        }
        hdr.ethernet.etherType = tmp_3;
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

