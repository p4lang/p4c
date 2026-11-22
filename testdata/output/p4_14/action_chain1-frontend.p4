#include <core.p4>
#define V1MODEL_VERSION 20200408
#include <v1model.p4>

header data_t {
    bit<32> f1;
    bit<32> f2;
    bit<16> h1;
    bit<8>  b1;
    bit<8>  b2;
}

header extra_t {
    bit<16> h;
    bit<8>  b1;
    bit<8>  b2;
}

struct metadata {
}

struct headers {
    @name(".data")
    data_t     data;
    @name(".extra")
    extra_t[4] extra;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".extra") state extra {
        packet.extract<extra_t>(hdr.extra.next);
        transition select(hdr.extra.last.b2) {
            8w0x80 &&& 8w0x80: extra;
            default: accept;
        }
    }
    @name(".start") state start {
        packet.extract<data_t>(hdr.data);
        transition extra;
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_2() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_3() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_4() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_5() {
    }
    @name(".set0b1") action set0b1(@name("val") bit<8> val) {
        hdr.extra[0].b1 = val;
    }
    @name(".act1") action act1(@name("val") bit<8> val_8) {
        hdr.extra[0].b1 = val_8;
    }
    @name(".act2") action act2(@name("val") bit<8> val_9) {
        hdr.extra[0].b1 = val_9;
    }
    @name(".act3") action act3(@name("val") bit<8> val_10) {
        hdr.extra[0].b1 = val_10;
    }
    @name(".noop") action noop() {
    }
    @name(".noop") action noop_1() {
    }
    @name(".noop") action noop_2() {
    }
    @name(".noop") action noop_3() {
    }
    @name(".noop") action noop_4() {
    }
    @name(".setb2") action setb2(@name("val") bit<8> val_11) {
        hdr.data.b2 = val_11;
    }
    @name(".set1b1") action set1b1(@name("val") bit<8> val_12) {
        hdr.extra[1].b1 = val_12;
    }
    @name(".set2b2") action set2b2(@name("val") bit<8> val_13) {
        hdr.extra[2].b2 = val_13;
    }
    @name(".setb1") action setb1(@name("port") bit<9> port, @name("val") bit<8> val_14) {
        hdr.data.b1 = val_14;
        standard_metadata.egress_spec = port;
    }
    @name(".ex1") table ex1_0 {
        actions = {
            set0b1();
            act1();
            act2();
            act3();
            noop();
            @defaultonly NoAction_1();
        }
        key = {
            hdr.extra[0].h: ternary @name("extra[0].h");
        }
        default_action = NoAction_1();
    }
    @name(".tbl1") table tbl1_0 {
        actions = {
            setb2();
            noop_1();
            @defaultonly NoAction_2();
        }
        key = {
            hdr.data.f2: ternary @name("data.f2");
        }
        default_action = NoAction_2();
    }
    @name(".tbl2") table tbl2_0 {
        actions = {
            set1b1();
            noop_2();
            @defaultonly NoAction_3();
        }
        key = {
            hdr.data.f2: ternary @name("data.f2");
        }
        default_action = NoAction_3();
    }
    @name(".tbl3") table tbl3_0 {
        actions = {
            set2b2();
            noop_3();
            @defaultonly NoAction_4();
        }
        key = {
            hdr.data.f2: ternary @name("data.f2");
        }
        default_action = NoAction_4();
    }
    @name(".test1") table test1_0 {
        actions = {
            setb1();
            noop_4();
            @defaultonly NoAction_5();
        }
        key = {
            hdr.data.f1: ternary @name("data.f1");
        }
        default_action = NoAction_5();
    }
    apply {
        test1_0.apply();
        switch (ex1_0.apply().action_run) {
            act1: {
                tbl1_0.apply();
            }
            act2: {
                tbl2_0.apply();
            }
            act3: {
                tbl3_0.apply();
            }
            default: {
            }
        }
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        packet.emit<data_t>(hdr.data);
        packet.emit<extra_t[4]>(hdr.extra);
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
