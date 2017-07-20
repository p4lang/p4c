#include <core.p4>
#include <v1model.p4>

header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> ethertype;
}

struct __metadataImpl {
    @name("standard_metadata") 
    standard_metadata_t standard_metadata;
}

struct __headersImpl {
    @name("ethernet") 
    ethernet_t ethernet;
}

parser __ParserImpl(packet_in packet, out __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t __standard_metadata) {
    @name(".start") state start {
        packet.extract<ethernet_t>(hdr.ethernet);
        transition accept;
    }
}

control egress(inout __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t __standard_metadata) {
    apply {
    }
}

control ingress(inout __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t __standard_metadata) {
    @name(".do_b") action do_b() {
    }
    @name(".do_d") action do_d() {
    }
    @name(".do_e") action do_e() {
    }
    @name(".nop") action nop() {
    }
    @name(".A") table A {
        actions = {
            do_b();
            do_d();
            do_e();
            @defaultonly NoAction();
        }
        key = {
            hdr.ethernet.dstAddr: exact @name("hdr.ethernet.dstAddr") ;
        }
        default_action = NoAction();
    }
    @name(".B") table B {
        actions = {
            nop();
            @defaultonly NoAction();
        }
        default_action = NoAction();
    }
    @name(".C") table C {
        actions = {
            nop();
            @defaultonly NoAction();
        }
        default_action = NoAction();
    }
    @name(".D") table D {
        actions = {
            nop();
            @defaultonly NoAction();
        }
        default_action = NoAction();
    }
    @name(".E") table E {
        actions = {
            nop();
            @defaultonly NoAction();
        }
        default_action = NoAction();
    }
    @name(".F") table F {
        actions = {
            nop();
            @defaultonly NoAction();
        }
        default_action = NoAction();
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

control __egressImpl(inout __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t __standard_metadata) {
    apply {
    }
}

control __DeparserImpl(packet_out packet, in __headersImpl hdr) {
    apply {
        packet.emit<ethernet_t>(hdr.ethernet);
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

V1Switch<__headersImpl, __metadataImpl>(__ParserImpl(), __verifyChecksumImpl(), ingress(), __egressImpl(), __computeChecksumImpl(), __DeparserImpl()) main;
