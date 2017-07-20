#include <core.p4>
#include <v1model.p4>

struct metadata_t {
    bit<1> field_1_1_1;
    bit<1> field_2_1_1;
}

struct __metadataImpl {
    @name("md") 
    metadata_t          md;
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

control egress(inout __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t __standard_metadata) {
    apply {
    }
}

control ingress(inout __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t __standard_metadata) {
    @name(".action_1_1") action action_1_1(bit<1> value) {
        meta.md.field_1_1_1 = value;
        meta.md.field_2_1_1 = 1w1;
    }
    @name(".dmac") table dmac {
        actions = {
            action_1_1();
            @defaultonly NoAction();
        }
        key = {
        }
        default_action = NoAction();
    }
    apply {
        dmac.apply();
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
