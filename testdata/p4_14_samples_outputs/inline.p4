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
    @name(".x") action x() {
    }
    @name(".t") table t {
        actions = {
            x;
        }
    }
    apply {
        if (meta.m.b == 1w1) {
            t.apply();
        }
    }
}

control d(inout __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t __standard_metadata) {
    @name(".c") c() c_0;
    apply {
        c_0.apply(hdr, meta, __standard_metadata);
    }
}

control ingress(inout __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t __standard_metadata) {
    @name(".d") d() d_0;
    apply {
        d_0.apply(hdr, meta, __standard_metadata);
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

V1Switch(__ParserImpl(), __verifyChecksumImpl(), ingress(), __egressImpl(), __computeChecksumImpl(), __DeparserImpl()) main;
