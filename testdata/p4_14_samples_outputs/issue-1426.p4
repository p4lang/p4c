#include <core.p4>
#include <v1model.p4>

header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
}

struct metadata {
}

struct headers {
    @name(".ethernet") 
    ethernet_t ethernet;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".start") state start {
        packet.extract(hdr.ethernet);
        transition accept;
    }
}

control c(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".send") action send(bit<9> port) {
        standard_metadata.egress_port = port;
    }
    @name(".discard") action discard() {
        mark_to_drop();
    }
    @name(".c1") table c1 {
        actions = {
            send;
            discard;
        }
        key = {
            hdr.ethernet.dstAddr: exact;
        }
        size = 1024;
    }
    @name(".c2") table c2 {
        actions = {
            send;
            discard;
        }
        key = {
            hdr.ethernet.dstAddr: exact;
        }
        size = 1024;
    }
    apply {
        if (standard_metadata.ingress_port & 9w0x2 == 9w1) {
            c1.apply();
        }
        if (standard_metadata.ingress_port & 9w0x4 == 9w1) {
            c2.apply();
        }
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".send") action send(bit<9> port) {
        standard_metadata.egress_port = port;
    }
    @name(".discard") action discard() {
        mark_to_drop();
    }
    @name(".a1") table a1 {
        actions = {
            send;
            discard;
        }
        key = {
            hdr.ethernet.dstAddr: exact;
        }
        size = 1024;
    }
    @name(".b1") table b1 {
        actions = {
            send;
            discard;
        }
        key = {
            hdr.ethernet.dstAddr: exact;
        }
        size = 1024;
    }
    @name(".c") c() c_0;
    apply {
        if (standard_metadata.ingress_port & 9w0x1 == 9w1) {
            a1.apply();
            c_0.apply(hdr, meta, standard_metadata);
        }
        else {
            b1.apply();
            c_0.apply(hdr, meta, standard_metadata);
        }
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        packet.emit(hdr.ethernet);
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

V1Switch(ParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;

