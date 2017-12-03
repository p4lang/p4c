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
        packet.extract(hdr.data2);
        transition d3;
    }
    @name(".d3") state d3 {
        packet.extract(hdr.data3);
        transition more;
    }
    @name(".more") state more {
        packet.extract(hdr.data);
        transition accept;
    }
    @name(".start") state start {
        packet.extract(hdr.data1);
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
            setb1;
            noop;
        }
        key = {
            hdr.data1.f1: exact;
            hdr.data2.f2: exact;
            hdr.data3.f3: exact;
        }
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
        packet.emit(hdr.data1);
        packet.emit(hdr.data2);
        packet.emit(hdr.data3);
        packet.emit(hdr.data);
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

V1Switch(ParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;

