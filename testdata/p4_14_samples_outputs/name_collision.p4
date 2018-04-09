#include <core.p4>
#include <v1model.p4>

struct B {
    bit<9>  A;
    bit<10> B;
}

@name("A") header A_0 {
    bit<32> A;
    bit<32> B;
    bit<16> h1;
    bit<8>  b1;
    bit<8>  b2;
}

struct metadata {
    @name(".meta") 
    B meta;
}

struct headers {
    @name(".A") 
    A_0 A;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".A") state A {
        packet.extract(hdr.A);
        transition accept;
    }
    @name(".start") state start {
        transition A;
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".B") counter(32w1024, CounterType.packets_and_bytes) B_1;
    @name(".A") action A_3(bit<8> val, bit<9> port, bit<10> idx) {
        hdr.A.b1 = val;
        standard_metadata.egress_spec = port;
        meta.meta.B = idx;
    }
    @name(".noop") action noop() {
    }
    @name(".B") action B_2() {
        B_1.count((bit<32>)(bit<32>)meta.meta.B);
    }
    @name(".A") table A_4 {
        actions = {
            A_3;
            noop;
        }
        key = {
            hdr.A.A: exact;
        }
    }
    @name(".B") table B_3 {
        actions = {
            B_2;
        }
        default_action = B_2();
    }
    apply {
        A_4.apply();
        B_3.apply();
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        packet.emit(hdr.A);
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

