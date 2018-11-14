#include <core.p4>
#include <v1model.p4>

struct ingress_metadata_t {
    bit<8>  drop;
    bit<8>  egress_port;
    bit<8>  f1;
    bit<16> f2;
    bit<32> f3;
    bit<64> f4;
}

header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> ethertype;
}

struct metadata {
    @name(".ing_metadata") 
    ingress_metadata_t ing_metadata;
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

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".nop") action nop() {
    }
    @name(".e_t1") table e_t1 {
        actions = {
            nop;
        }
        key = {
            hdr.ethernet.srcAddr: exact;
        }
    }
    apply {
        e_t1.apply();
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".nop") action nop() {
    }
    @name(".ing_drop") action ing_drop() {
        meta.ing_metadata.drop = 8w1;
    }
    @name(".set_f1") action set_f1(bit<8> f1) {
        meta.ing_metadata.f1 = f1;
    }
    @name(".set_f2") action set_f2(bit<16> f2) {
        meta.ing_metadata.f2 = f2;
    }
    @name(".set_f3") action set_f3(bit<32> f3) {
        meta.ing_metadata.f3 = f3;
    }
    @name(".set_egress_port") action set_egress_port(bit<8> egress_port) {
        meta.ing_metadata.egress_port = egress_port;
    }
    @name(".set_f4") action set_f4(bit<64> f4) {
        meta.ing_metadata.f4 = f4;
    }
    @name(".i_t1") table i_t1 {
        actions = {
            nop;
            ing_drop;
            set_f1;
            set_f2;
            set_f3;
            set_egress_port;
        }
        key = {
            hdr.ethernet.dstAddr: exact;
        }
    }
    @name(".i_t2") table i_t2 {
        actions = {
            nop;
            set_f2;
        }
        key = {
            hdr.ethernet.dstAddr: exact;
        }
    }
    @name(".i_t3") table i_t3 {
        actions = {
            nop;
            set_f3;
        }
        key = {
            hdr.ethernet.dstAddr: exact;
        }
    }
    @name(".i_t4") table i_t4 {
        actions = {
            nop;
            set_f4;
        }
        key = {
            hdr.ethernet.dstAddr: exact;
        }
    }
    apply {
        switch (i_t1.apply().action_run) {
            nop: {
                i_t2.apply();
            }
            set_egress_port: {
                i_t3.apply();
            }
            default: {
                i_t4.apply();
            }
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

