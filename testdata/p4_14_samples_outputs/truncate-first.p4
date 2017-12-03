#include <core.p4>
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
    @name("._nop") action _nop() {
    }
    @name("._truncate") action _truncate(bit<32> new_length, bit<9> port) {
        standard_metadata.egress_spec = port;
        truncate(new_length);
    }
    @name(".t_ingress") table t_ingress {
        actions = {
            _nop();
            _truncate();
            @defaultonly NoAction();
        }
        key = {
            hdr.hdrA.f1: exact @name("hdrA.f1") ;
        }
        size = 128;
        default_action = NoAction();
    }
    apply {
        t_ingress.apply();
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

