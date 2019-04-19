#include <core.p4>
#include <v1model.p4>

struct ingress_metadata_t {
    bit<1>    drop;
    bit<8>    egress_port;
    bit<1024> f1;
    bit<512>  f2;
    bit<256>  f3;
    bit<128>  f4;
}

header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> ethertype;
}

header vag_t {
    bit<1024> f1;
    bit<512>  f2;
    bit<256>  f3;
    bit<128>  f4;
}

struct metadata {
    @name(".ing_metadata") 
    ingress_metadata_t ing_metadata;
}

struct headers {
    @name(".ethernet") 
    ethernet_t ethernet;
    @name(".vag") 
    vag_t      vag;
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
    @name(".set_f1") action set_f1(bit<1024> f1) {
        meta.ing_metadata.f1 = f1;
    }
    @name(".set_f2") action set_f2(bit<512> f2) {
        meta.ing_metadata.f2 = f2;
    }
    @name(".set_f3") action set_f3(bit<256> f3) {
        meta.ing_metadata.f3 = f3;
    }
    @name(".set_f4") action set_f4(bit<128> f4) {
        meta.ing_metadata.f4 = f4;
    }
    @name(".i_t1") table i_t1 {
        actions = {
            nop;
            set_f1;
        }
        key = {
            hdr.vag.f1: exact;
        }
    }
    @name(".i_t2") table i_t2 {
        actions = {
            nop;
            set_f2;
        }
        key = {
            hdr.vag.f2: exact;
        }
    }
    @name(".i_t3") table i_t3 {
        actions = {
            nop;
            set_f3;
        }
        key = {
            hdr.vag.f3: exact;
        }
    }
    @name(".i_t4") table i_t4 {
        actions = {
            nop;
            set_f4;
        }
        key = {
            hdr.vag.f4: ternary;
        }
    }
    apply {
        i_t1.apply();
        i_t2.apply();
        i_t3.apply();
        i_t4.apply();
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

