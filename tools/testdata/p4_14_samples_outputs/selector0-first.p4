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
    @name(".data") 
    data_t data;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".start") state start {
        packet.extract<data_t>(hdr.data);
        transition accept;
    }
}

@name(".sel_profile") @mode("fair") action_selector(HashAlgorithm.crc16, 32w16384, 32w14) sel_profile;

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".noop") action noop() {
    }
    @name(".test1") table test1 {
        actions = {
            noop();
            @defaultonly NoAction();
        }
        key = {
            hdr.data.b1: exact @name("data.b1") ;
            hdr.data.f1: selector @name("data.f1") ;
            hdr.data.f2: selector @name("data.f2") ;
            hdr.data.f3: selector @name("data.f3") ;
            hdr.data.f4: selector @name("data.f4") ;
        }
        size = 1024;
        implementation = sel_profile;
        default_action = NoAction();
    }
    apply {
        test1.apply();
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

control verifyChecksum(inout headers hdr, inout metadata meta) {
    apply {
    }
}

control computeChecksum(inout headers hdr, inout metadata meta) {
    apply {
    }
}

V1Switch<headers, metadata>(ParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;

