#include <core.p4>
#include <v1model.p4>

header data_t {
    bit<32> f1;
    bit<32> f2;
    bit<32> f3;
    bit<32> f4;
    bit<8>  b1;
    bit<8>  b2;
    bit<8>  b3;
    bit<8>  b4;
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
    @name(".noop") action noop_0() {
    }
    @name(".setf1") action setf1_0(bit<32> val) {
        hdr.data.f1 = val;
    }
    @name(".test1") table test1_0 {
        actions = {
            noop_0();
            setf1_0();
            @defaultonly NoAction();
        }
        key = {
            hdr.data.b1: exact @name("hdr.data.b1") ;
            hdr.data.f1: selector @name("hdr.data.f1") ;
            hdr.data.f2: selector @name("hdr.data.f2") ;
            hdr.data.f3: selector @name("hdr.data.f3") ;
            hdr.data.f4: selector @name("hdr.data.f4") ;
        }
        size = 1024;
        @name(".sel_profile") @mode("fair") implementation = action_selector(HashAlgorithm.crc16, 32w16384, 32w14);
        default_action = NoAction();
    }
    apply {
        test1_0.apply();
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
