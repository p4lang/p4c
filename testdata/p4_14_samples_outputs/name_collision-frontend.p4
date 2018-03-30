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
        packet.extract<A_0>(hdr.A);
        transition accept;
    }
    @name(".start") state start {
        transition A;
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".NoAction") action NoAction_0() {
    }
    @name(".B") counter(32w1024, CounterType.packets_and_bytes) B_2;
    @name(".A") action A_1(bit<8> val, bit<9> port, bit<10> idx) {
        hdr.A.b1 = val;
        standard_metadata.egress_spec = port;
        meta.meta.B = idx;
    }
    @name(".noop") action noop_0() {
    }
    @name(".B") action B_3() {
        B_2.count((bit<32>)meta.meta.B);
    }
    @name(".A") table A_4 {
        actions = {
            A_1();
            noop_0();
            @defaultonly NoAction_0();
        }
        key = {
            hdr.A.A: exact @name("A.A") ;
        }
        default_action = NoAction_0();
    }
    @name(".B") table B_4 {
        actions = {
            B_3();
        }
        default_action = B_3();
    }
    apply {
        A_4.apply();
        B_4.apply();
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        packet.emit<A_0>(hdr.A);
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

