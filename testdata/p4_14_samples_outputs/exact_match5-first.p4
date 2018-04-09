#include <core.p4>
#include <v1model.p4>

header data_t {
    bit<128> f4;
    bit<8>   b1;
    bit<8>   b2;
}

header data1_t {
    bit<128> f1;
}

header data2_t {
    bit<128> f2;
}

header data3_t {
    bit<128> f3;
}

struct metadata {
}

struct headers {
    @name(".data") 
    data_t  data;
    @name(".data1") 
    data1_t data1;
    @name(".data2") 
    data2_t data2;
    @name(".data3") 
    data3_t data3;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".d2") state d2 {
        packet.extract<data2_t>(hdr.data2);
        transition d3;
    }
    @name(".d3") state d3 {
        packet.extract<data3_t>(hdr.data3);
        transition more;
    }
    @name(".more") state more {
        packet.extract<data_t>(hdr.data);
        transition accept;
    }
    @name(".start") state start {
        packet.extract<data1_t>(hdr.data1);
        transition d2;
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".setb1") action setb1(bit<8> val, bit<9> port) {
        hdr.data.b1 = val;
        standard_metadata.egress_spec = port;
    }
    @name(".noop") action noop() {
    }
    @name(".test1") table test1 {
        actions = {
            setb1();
            noop();
            @defaultonly NoAction();
        }
        key = {
            hdr.data1.f1: exact @name("data1.f1") ;
            hdr.data2.f2: exact @name("data2.f2") ;
            hdr.data3.f3: exact @name("data3.f3") ;
        }
        size = 100000;
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
        packet.emit<data1_t>(hdr.data1);
        packet.emit<data2_t>(hdr.data2);
        packet.emit<data3_t>(hdr.data3);
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

