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

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".setf1") action setf1(bit<32> val) {
        hdr.data.f1 = val;
    }
    @name(".noop") action noop() {
    }
    @name(".setb4") action setb4(bit<32> val) {
        hdr.data.b4 = val;
    }
    @name(".setb1") action setb1(bit<32> val) {
        hdr.data.b1 = val;
    }
    @name(".E1") table E1 {
        actions = {
            setf1();
            noop();
            @defaultonly NoAction();
        }
        key = {
            hdr.data.f2: ternary @name("data.f2") ;
        }
        default_action = NoAction();
    }
    @name(".E2") table E2 {
        actions = {
            setb4();
            noop();
            @defaultonly NoAction();
        }
        key = {
            hdr.data.b1: ternary @name("data.b1") ;
        }
        default_action = NoAction();
    }
    @name(".EA") table EA {
        actions = {
            setb1();
            noop();
            @defaultonly NoAction();
        }
        key = {
            hdr.data.f3: ternary @name("data.f3") ;
        }
        default_action = NoAction();
    }
    @name(".EB") table EB {
        actions = {
            setb1();
            noop();
            @defaultonly NoAction();
        }
        key = {
            hdr.data.f4: ternary @name("data.f4") ;
        }
        default_action = NoAction();
    }
    apply {
        E1.apply();
        if (hdr.data.f1 == 32w0) 
            EA.apply();
        else 
            EB.apply();
        E2.apply();
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".setb1") action setb1(bit<32> val) {
        hdr.data.b1 = val;
    }
    @name(".noop") action noop() {
    }
    @name(".setb3") action setb3(bit<32> val) {
        hdr.data.b3 = val;
    }
    @name(".setb2") action setb2(bit<32> val) {
        hdr.data.b2 = val;
    }
    @name(".setb4") action setb4(bit<32> val) {
        hdr.data.b4 = val;
    }
    @name(".A1") table A1 {
        actions = {
            setb1();
            noop();
            @defaultonly NoAction();
        }
        key = {
            hdr.data.f1: ternary @name("data.f1") ;
        }
        default_action = NoAction();
    }
    @name(".A2") table A2 {
        actions = {
            setb3();
            noop();
            @defaultonly NoAction();
        }
        key = {
            hdr.data.b1: ternary @name("data.b1") ;
        }
        default_action = NoAction();
    }
    @name(".A3") table A3 {
        actions = {
            setb1();
            noop();
            @defaultonly NoAction();
        }
        key = {
            hdr.data.b3: ternary @name("data.b3") ;
        }
        default_action = NoAction();
    }
    @name(".B1") table B1 {
        actions = {
            setb2();
            noop();
            @defaultonly NoAction();
        }
        key = {
            hdr.data.f2: ternary @name("data.f2") ;
        }
        default_action = NoAction();
    }
    @name(".B2") table B2 {
        actions = {
            setb4();
            noop();
            @defaultonly NoAction();
        }
        key = {
            hdr.data.b2: ternary @name("data.b2") ;
        }
        default_action = NoAction();
    }
    apply {
        if (hdr.data.b1 == 32w0) {
            A1.apply();
            A2.apply();
            A3.apply();
        }
        B1.apply();
        B2.apply();
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

