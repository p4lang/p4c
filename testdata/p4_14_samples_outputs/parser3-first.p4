#include <core.p4>
#include <v1model.p4>

header data1_t {
    bit<32> f1;
    bit<2>  x1;
    bit<4>  x2;
    bit<4>  x3;
    bit<4>  x4;
    bit<2>  x5;
    bit<5>  x6;
    bit<2>  x7;
    bit<1>  x8;
}

header data2_t {
    bit<8> a1;
    bit<4> a2;
    bit<4> a3;
    bit<8> a4;
    bit<4> a5;
    bit<4> a6;
}

struct __metadataImpl {
    @name("standard_metadata") 
    standard_metadata_t standard_metadata;
}

struct __headersImpl {
    @name("data1") 
    data1_t data1;
    @name("data2") 
    data2_t data2;
}

parser __ParserImpl(packet_in packet, out __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t __standard_metadata) {
    @name(".parse_data2") state parse_data2 {
        packet.extract<data2_t>(hdr.data2);
        transition accept;
    }
    @name(".start") state start {
        packet.extract<data1_t>(hdr.data1);
        transition select(hdr.data1.x3, hdr.data1.x1, hdr.data1.x7) {
            (4w0xe, 2w0x1, 2w0x0): parse_data2;
            default: accept;
        }
    }
}

control ingress(inout __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t __standard_metadata) {
    @name(".noop") action noop() {
    }
    @name(".test1") table test1 {
        actions = {
            noop();
            @defaultonly NoAction();
        }
        key = {
            hdr.data1.f1: exact @name("hdr.data1.f1") ;
        }
        default_action = NoAction();
    }
    apply {
        test1.apply();
    }
}

control __egressImpl(inout __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t __standard_metadata) {
    apply {
    }
}

control __DeparserImpl(packet_out packet, in __headersImpl hdr) {
    apply {
        packet.emit<data1_t>(hdr.data1);
        packet.emit<data2_t>(hdr.data2);
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
