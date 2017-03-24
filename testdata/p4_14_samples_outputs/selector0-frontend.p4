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

struct metadata {
}

struct headers {
    @name("data")
    data_t data;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name("start") state start {
        packet.extract<data_t>(hdr.data);
        transition accept;
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name("noop") action noop_0() {
    }
    @name("test1") table test1_0 {
        actions = {
            noop_0();
            @default_only NoAction();
        }
        key = {
            hdr.data.b1: exact @name("hdr.data.b1") ;
            hdr.data.f1: selector @name("hdr.data.f1") ;
            hdr.data.f2: selector @name("hdr.data.f2") ;
            hdr.data.f3: selector @name("hdr.data.f3") ;
            hdr.data.f4: selector @name("hdr.data.f4") ;
        }
        size = 1024;
        default_action = NoAction();
        @name("sel_profile") @mode("fair") implementation = action_selector(HashAlgorithm.crc16, 32w16384, 32w14);
    }
    apply {
        test1_0.apply();
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        packet.emit<data_t>(hdr.data);
    }
}

control verifyChecksum(in headers hdr, inout metadata meta) {
    apply {
    }
}

control computeChecksum(inout headers hdr, inout metadata meta) {
    apply {
    }
}

V1Switch<headers, metadata>(ParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;
