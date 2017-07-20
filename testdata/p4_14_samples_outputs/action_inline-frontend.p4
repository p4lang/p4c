#include <core.p4>
#include <v1model.p4>

struct ht {
    bit<1> b;
}

struct __metadataImpl {
    @name("md") 
    ht                  md;
    @name("standard_metadata") 
    standard_metadata_t standard_metadata;
}

struct __headersImpl {
}

parser __ParserImpl(packet_in packet, out __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t __standard_metadata) {
    @name(".start") state start {
        transition accept;
    }
}

control ingress(inout __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t __standard_metadata) {
    @name(".a") action a_0(inout bit<1> y0_0) {
        y0_0 = y0_0 + 1w1;
    }
    @name(".b") action b_0() {
        a_0(meta.md.b);
        a_0(meta.md.b);
    }
    @name(".t") table t_0 {
        actions = {
            b_0();
            @defaultonly NoAction();
        }
        default_action = NoAction();
    }
    apply {
        t_0.apply();
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
