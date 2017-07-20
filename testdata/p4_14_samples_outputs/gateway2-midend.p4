#include <core.p4>
#include <v1model.p4>

header data_t {
    bit<32> f1;
    bit<32> f2;
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
    @name("NoAction") action NoAction_0() {
    }
    @name("NoAction") action NoAction_3() {
    }
    @name(".setb1") action setb1_0(bit<8> val, bit<9> port) {
        hdr.data.b1 = val;
        meta.standard_metadata.egress_spec = port;
    }
    @name(".setb1") action setb1_2(bit<8> val, bit<9> port) {
        hdr.data.b1 = val;
        meta.standard_metadata.egress_spec = port;
    }
    @name(".noop") action noop_0() {
    }
    @name(".noop") action noop_2() {
    }
    @name(".test1") table test1 {
        actions = {
            setb1_0();
            noop_0();
            @defaultonly NoAction_0();
        }
        key = {
            hdr.data.f1: exact @name("hdr.data.f1") ;
        }
        default_action = NoAction_0();
    }
    @name(".test2") table test2 {
        actions = {
            setb1_2();
            noop_2();
            @defaultonly NoAction_3();
        }
        key = {
            hdr.data.f2: exact @name("hdr.data.f2") ;
        }
        default_action = NoAction_3();
    }
    apply {
        if (hdr.data.b2 == hdr.data.b3 && hdr.data.b4 == 8w10) 
            test1.apply();
        else 
            test2.apply();
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
