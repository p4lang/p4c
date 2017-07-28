#include <core.p4>
#include <v1model.p4>

header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> ethertype;
}

struct __metadataImpl {
}

struct __headersImpl {
    @name("ethernet") 
    ethernet_t ethernet;
}

parser __ParserImpl(packet_in packet, out __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t standard_metadata) {
    @name(".start") state start {
        packet.extract(hdr.ethernet);
        transition accept;
    }
}

control egress(inout __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control ingress(inout __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t standard_metadata) {
    @name(".do_b") action do_b() {
        ;
    }
    @name(".do_d") action do_d() {
        ;
    }
    @name(".do_e") action do_e() {
        ;
    }
    @name(".nop") action nop() {
        ;
    }
    @name(".A") table A {
        actions = {
            do_b;
            do_d;
            do_e;
        }
        key = {
            hdr.ethernet.dstAddr: exact;
        }
    }
    @name(".B") table B {
        actions = {
            nop;
        }
    }
    @name(".C") table C {
        actions = {
            nop;
        }
    }
    @name(".D") table D {
        actions = {
            nop;
        }
    }
    @name(".E") table E {
        actions = {
            nop;
        }
    }
    @name(".F") table F {
        actions = {
            nop;
        }
    }
    apply {
        switch (A.apply().action_run) {
            do_b: {
                B.apply();
                C.apply();
            }
            do_d: {
                D.apply();
                C.apply();
            }
            do_e: {
                E.apply();
            }
        }

        F.apply();
    }
}

control __DeparserImpl(packet_out packet, in __headersImpl hdr) {
    apply {
        packet.emit(hdr.ethernet);
    }
}

control __verifyChecksumImpl(in __headersImpl hdr, inout __metadataImpl meta) {
    apply {
    }
}

control __computeChecksumImpl(inout __headersImpl hdr, inout __metadataImpl meta) {
    apply {
    }
}

V1Switch(__ParserImpl(), __verifyChecksumImpl(), ingress(), egress(), __computeChecksumImpl(), __DeparserImpl()) main;
