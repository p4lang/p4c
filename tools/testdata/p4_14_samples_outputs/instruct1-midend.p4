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

@name("data2_t") header data2_t_0 {
    bit<16> x1;
    bit<16> x2;
}

struct metadata {
}

struct headers {
    @name(".data") 
    data_t    data;
    @name(".hdr1") 
    data2_t_0 hdr1;
    @name(".hdr2") 
    data2_t_0 hdr2;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".parse_data2") state parse_data2 {
        packet.extract<data2_t_0>(hdr.hdr1);
        transition select(hdr.hdr1.x1) {
            16w1 &&& 16w1: parse_hdr2;
            default: accept;
        }
    }
    @name(".parse_hdr2") state parse_hdr2 {
        packet.extract<data2_t_0>(hdr.hdr2);
        transition accept;
    }
    @name(".start") state start {
        packet.extract<data_t>(hdr.data);
        transition select(hdr.data.b1) {
            8w0x0: parse_data2;
            default: accept;
        }
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".NoAction") action NoAction_0() {
    }
    @name(".decap") action decap() {
        hdr.hdr1 = hdr.hdr2;
        hdr.hdr2.setInvalid();
    }
    @name(".noop") action noop() {
    }
    @name(".test1") table test1_0 {
        actions = {
            decap();
            noop();
            @defaultonly NoAction_0();
        }
        key = {
            hdr.data.f1: exact @name("data.f1") ;
        }
        default_action = NoAction_0();
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
        packet.emit<data2_t_0>(hdr.hdr1);
        packet.emit<data2_t_0>(hdr.hdr2);
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

