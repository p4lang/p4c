#include <core.p4>
#include <v1model.p4>

struct m_t {
    bit<32> f1;
}

struct __metadataImpl {
    @name("m") 
    m_t                 m;
    @name("standard_metadata") 
    standard_metadata_t standard_metadata;
}

struct __headersImpl {
}

parser __ParserImpl(packet_in packet, out __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t __standard_metadata) {
    @name(".start") state start {
        meta.m.f1 = 32w2;
        transition accept;
    }
}

control ingress(inout __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t __standard_metadata) {
    @name("NoAction") action NoAction_0() {
    }
    @name(".a1") action a1_0() {
        meta.m.f1 = 32w1;
    }
    @name(".t1") table t1 {
        actions = {
            a1_0();
            @defaultonly NoAction_0();
        }
        default_action = NoAction_0();
    }
    apply {
        t1.apply();
    }
}

control __egressImpl(inout __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t __standard_metadata) {
    apply {
    }
}

control __DeparserImpl(packet_out packet, in __headersImpl hdr) {
    apply {
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
