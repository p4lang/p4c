#include "/home/mbudiu/barefoot/git/p4c/build/../p4include/core.p4"
#include "/home/mbudiu/barefoot/git/p4c/build/../p4include/v1model.p4"

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
    @name("data") 
    data_t     data;
    @name("extra") 
    extra_t[4] extra;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name("extra") state extra {
        packet.extract(hdr.extra.next);
        transition select(hdr.extra.last.b2) {
            8w0x80 &&& 8w0x80: extra;
            default: accept;
        }
    }
    @name("start") state start {
        packet.extract(hdr.data);
        transition extra;
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    action NoAction_0() {
    }
    @name("set0b1") action set0b1_0(bit<8> val) {
        hdr.extra[0].b1 = val;
    }
    @name("act1") action act1_0(bit<8> val) {
        hdr.extra[0].b1 = val;
    }
    @name("act2") action act2_0(bit<8> val) {
        hdr.extra[0].b1 = val;
    }
    @name("act3") action act3_0(bit<8> val) {
        hdr.extra[0].b1 = val;
    }
    @name("noop") action noop_0() {
    }
    @name("setb2") action setb2_0(bit<8> val) {
        hdr.data.b2 = val;
    }
    @name("set1b1") action set1b1_0(bit<8> val) {
        hdr.extra[1].b1 = val;
    }
    @name("set2b2") action set2b2_0(bit<8> val) {
        hdr.extra[2].b2 = val;
    }
    @name("setb1") action setb1_0(bit<9> port, bit<8> val) {
        hdr.data.b1 = val;
        standard_metadata.egress_spec = port;
    }
    @name("ex1") table ex1_0() {
        actions = {
            set0b1_0;
            act1_0;
            act2_0;
            act3_0;
            noop_0;
            NoAction_0;
        }
        key = {
            hdr.extra[0].h: ternary;
        }
        default_action = NoAction_0();
    }
    @name("tbl1") table tbl1_0() {
        actions = {
            setb2_0;
            noop_0;
            NoAction_0;
        }
        key = {
            hdr.data.f2: ternary;
        }
        default_action = NoAction_0();
    }
    @name("tbl2") table tbl2_0() {
        actions = {
            set1b1_0;
            noop_0;
            NoAction_0;
        }
        key = {
            hdr.data.f2: ternary;
        }
        default_action = NoAction_0();
    }
    @name("tbl3") table tbl3_0() {
        actions = {
            set2b2_0;
            noop_0;
            NoAction_0;
        }
        key = {
            hdr.data.f2: ternary;
        }
        default_action = NoAction_0();
    }
    @name("test1") table test1_0() {
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
    apply {
        test1_0.apply();
        switch (ex1_0.apply().action_run) {
            act1_0: {
                tbl1_0.apply();
            }
            act2_0: {
                tbl2_0.apply();
            }
            act3_0: {
                tbl3_0.apply();
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
        packet.emit(hdr.data);
        packet.emit(hdr.extra);
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
