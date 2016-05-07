#include "/home/mbudiu/barefoot/git/p4c/build/../p4include/core.p4"
#include "/home/mbudiu/barefoot/git/p4c/build/../p4include/v1model.p4"

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
    @name("data") 
    data_t data;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name("start") state start {
        packet.extract(hdr.data);
        transition accept;
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    action NoAction_0() {
    }
    action NoAction_1() {
    }
    action NoAction_2() {
    }
    action NoAction_3() {
    }
    action NoAction_4() {
    }
    action NoAction_5() {
    }
    @name("setb1") action setb1_0(bit<32> val) {
        hdr.data.b1 = val;
    }
    @name("noop") action noop_0() {
    }
    @name("noop") action noop() {
    }
    @name("noop") action noop_1() {
    }
    @name("noop") action noop_2() {
    }
    @name("setb3") action setb3_0(bit<32> val) {
        hdr.data.b3 = val;
    }
    @name("on_hit") action on_hit_0() {
    }
    @name("on_hit") action on_hit() {
    }
    @name("on_miss") action on_miss_0() {
    }
    @name("on_miss") action on_miss() {
    }
    @name("setb2") action setb2_0(bit<32> val) {
        hdr.data.b2 = val;
    }
    @name("setb4") action setb4_0(bit<32> val) {
        hdr.data.b4 = val;
    }
    @name("A1") table A1_0() {
        actions = {
            setb1_0;
            noop_0;
            NoAction_0;
        }
        key = {
            hdr.data.f1: ternary;
        }
        default_action = NoAction_0();
    }
    @name("A2") table A2_0() {
        actions = {
            setb3_0;
            noop;
            NoAction_1;
        }
        key = {
            hdr.data.b1: ternary;
        }
        default_action = NoAction_0();
    }
    @name("A3") table A3_0() {
        actions = {
            on_hit_0;
            on_miss_0;
            NoAction_2;
        }
        key = {
            hdr.data.f2: ternary;
        }
        default_action = NoAction_0();
    }
    @name("A4") table A4_0() {
        actions = {
            on_hit;
            on_miss;
            NoAction_3;
        }
        key = {
            hdr.data.f2: ternary;
        }
        default_action = NoAction_0();
    }
    @name("B1") table B1_0() {
        actions = {
            setb2_0;
            noop_1;
            NoAction_4;
        }
        key = {
            hdr.data.f2: ternary;
        }
        default_action = NoAction_0();
    }
    @name("B2") table B2_0() {
        actions = {
            setb4_0;
            noop_2;
            NoAction_5;
        }
        key = {
            hdr.data.b2: ternary;
        }
        default_action = NoAction_0();
    }
    apply {
        if (hdr.data.b1 == 32w0) {
            A1_0.apply();
            A2_0.apply();
            if (hdr.data.f1 == 32w0) 
                switch (A3_0.apply().action_run) {
                    on_hit_0: {
                        A4_0.apply();
                    }
                }

        }
        B1_0.apply();
        B2_0.apply();
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        packet.emit(hdr.data);
    }
}

control verifyChecksum(in headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control computeChecksum(inout headers hdr, inout metadata meta) {
    apply {
    }
}

V1Switch(ParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;
