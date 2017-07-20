#include <core.p4>
#include <v1model.p4>

struct h {
    bit<1> b;
}

struct __metadataImpl {
    @name("m") 
    h                   m;
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

control c(inout __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t __standard_metadata) {
    @name(".x") action x_0() {
    }
    @name(".t") table t_0 {
        actions = {
            x_0();
            @defaultonly NoAction();
        }
        default_action = NoAction();
    }
    apply {
        if (meta.m.b == 1w1) 
            t_0.apply();
    }
}

control d(inout __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t __standard_metadata) {
    @name(".c") c() c_1;
    apply {
        c_1.apply(hdr, meta, __standard_metadata);
    }
}

control ingress(inout __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t __standard_metadata) {
    @name(".d") d() d_1;
    apply {
        d_1.apply(hdr, meta, __standard_metadata);
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
