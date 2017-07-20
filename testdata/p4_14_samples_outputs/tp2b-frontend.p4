#include <core.p4>
#include <v1model.p4>

header data_t {
    bit<32> f1;
    bit<32> f2;
    bit<32> f3;
    bit<32> f4;
    bit<32> b1;
    bit<32> b2;
    bit<32> b3;
    bit<32> b4;
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
    @name(".setb1") action setb1_0(bit<32> val) {
        hdr.data.b1 = val;
    }
    @name(".noop") action noop_0() {
    }
    @name(".setb3") action setb3_0(bit<32> val) {
        hdr.data.b3 = val;
    }
    @name(".setb2") action setb2_0(bit<32> val) {
        hdr.data.b2 = val;
    }
    @name(".setb4") action setb4_0(bit<32> val) {
        hdr.data.b4 = val;
    }
    @name(".A1") table A1_0 {
        actions = {
            setb1_0();
            noop_0();
            @defaultonly NoAction();
        }
        key = {
            hdr.data.f1: ternary @name("hdr.data.f1") ;
        }
        default_action = NoAction();
    }
    @name(".A2") table A2_0 {
        actions = {
            setb3_0();
            noop_0();
            @defaultonly NoAction();
        }
        key = {
            hdr.data.b1: ternary @name("hdr.data.b1") ;
        }
        default_action = NoAction();
    }
    @name(".B1") table B1_0 {
        actions = {
            setb2_0();
            noop_0();
            @defaultonly NoAction();
        }
        key = {
            hdr.data.f2: ternary @name("hdr.data.f2") ;
        }
        default_action = NoAction();
    }
    @name(".B2") table B2_0 {
        actions = {
            setb4_0();
            noop_0();
            @defaultonly NoAction();
        }
        key = {
            hdr.data.b2: ternary @name("hdr.data.b2") ;
        }
        default_action = NoAction();
    }
    apply {
        if (hdr.data.b1 == 32w0) {
            A1_0.apply();
            A2_0.apply();
        }
        B1_0.apply();
        B2_0.apply();
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
