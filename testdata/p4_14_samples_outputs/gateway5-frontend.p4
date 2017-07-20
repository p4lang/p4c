#include <core.p4>
#include <v1model.p4>

header data_t {
    bit<32> f1;
    bit<32> f2;
    bit<2>  x1;
    bit<3>  pad0;
    bit<2>  x2;
    bit<5>  pad1;
    bit<1>  x3;
    bit<3>  pad2;
    bit<32> skip;
    bit<1>  x4;
    bit<1>  x5;
    bit<6>  pad3;
}

struct __metadataImpl {
    @name("standard_metadata") 
    standard_metadata_t standard_metadata;
}

struct __headersImpl {
    @name("data") 
    data_t data;
}

parser __ParserImpl(packet_in packet, out __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t __standard_metadata) {
    @name(".start") state start {
        packet.extract<data_t>(hdr.data);
        transition accept;
    }
}

control ingress(inout __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t __standard_metadata) {
    @name(".output") action output_0(bit<9> port) {
        meta.standard_metadata.egress_spec = port;
    }
    @name(".noop") action noop_0() {
    }
    @name(".test1") table test1_0 {
        actions = {
            output_0();
            noop_0();
            @defaultonly NoAction();
        }
        key = {
            hdr.data.f1: exact @name("hdr.data.f1") ;
        }
        default_action = NoAction();
    }
    @name(".test2") table test2_0 {
        actions = {
            output_0();
            noop_0();
            @defaultonly NoAction();
        }
        key = {
            hdr.data.f2: exact @name("hdr.data.f2") ;
        }
        default_action = NoAction();
    }
    apply {
        if (hdr.data.x2 == 2w1 && hdr.data.x4 == 1w0) 
            test1_0.apply();
        else 
            test2_0.apply();
    }
}

control __egressImpl(inout __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t __standard_metadata) {
    apply {
    }
}

control __DeparserImpl(packet_out packet, in __headersImpl hdr) {
    apply {
        packet.emit<data_t>(hdr.data);
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
