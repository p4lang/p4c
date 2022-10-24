#include <core.p4>
#define V1MODEL_VERSION 20200408
#include <v1model.p4>

header hdrA_t {
    bit<8>  f1;
    bit<64> f2;
}

struct metadata {
}

struct headers {
    @name(".hdrA")
    hdrA_t hdrA;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".start") state start {
        packet.extract<hdrA_t>(hdr.hdrA);
        transition accept;
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @name("._nop") action _nop() {
    }
    @name("._truncate") action _truncate(@name("new_length") bit<32> new_length, @name("port") bit<9> port) {
        standard_metadata.egress_spec = port;
        truncate(new_length);
    }
    @name(".t_ingress") table t_ingress_0 {
        actions = {
            _nop();
            _truncate();
            @defaultonly NoAction_1();
        }
        key = {
            hdr.hdrA.f1: exact @name("hdrA.f1");
        }
        size = 128;
        default_action = NoAction_1();
    }
    apply {
        t_ingress_0.apply();
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        packet.emit<hdrA_t>(hdr.hdrA);
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
