#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

header data_h {
    bit<32> f1;
    bit<32> f2;
    bit<16> h1;
    bit<8>  b1;
    bit<8>  b2;
}

header extra_h {
    bit<16> h;
    bit<8>  b1;
    bit<8>  b2;
}

struct packet_t {
    data_h     data;
    extra_h[4] extra;
}

struct Meta {
}

parser p(packet_in b, out packet_t hdrs, inout Meta m, inout standard_metadata_t meta) {
    state start {
        b.extract<data_h>(hdrs.data);
        transition extra;
    }
    state extra {
        b.extract<extra_h>(hdrs.extra.next);
        transition select(hdrs.extra.last.b2) {
            8w0x80 &&& 8w0x80: extra;
            default: accept;
        }
    }
}

control vrfy(inout packet_t h, inout Meta m) {
    apply {
    }
}

control update(inout packet_t h, inout Meta m) {
    apply {
    }
}

control ingress(inout packet_t hdrs, inout Meta m, inout standard_metadata_t meta) {
    @name("ingress.setb1") action setb1(@name("port") bit<9> port, @name("val") bit<8> val) {
        hdrs.data.b1 = val;
        meta.egress_spec = port;
    }
    @name("ingress.noop") action noop() {
    }
    @name("ingress.noop") action noop_1() {
    }
    @name("ingress.noop") action noop_2() {
    }
    @name("ingress.noop") action noop_3() {
    }
    @name("ingress.noop") action noop_4() {
    }
    @name("ingress.setbyte") action setbyte(@name("val") bit<8> val_5) {
        hdrs.extra[0].b1 = val_5;
    }
    @name("ingress.setbyte") action setbyte_1(@name("val") bit<8> val_6) {
        hdrs.data.b2 = val_6;
    }
    @name("ingress.setbyte") action setbyte_2(@name("val") bit<8> val_7) {
        hdrs.extra[1].b1 = val_7;
    }
    @name("ingress.setbyte") action setbyte_3(@name("val") bit<8> val_8) {
        hdrs.extra[2].b2 = val_8;
    }
    @name("ingress.act1") action act1(@name("val") bit<8> val_9) {
        hdrs.extra[0].b1 = val_9;
    }
    @name("ingress.act2") action act2(@name("val") bit<8> val_10) {
        hdrs.extra[0].b1 = val_10;
    }
    @name("ingress.act3") action act3(@name("val") bit<8> val_11) {
        hdrs.extra[0].b1 = val_11;
    }
    @name("ingress.test1") table test1_0 {
        key = {
            hdrs.data.f1: ternary @name("hdrs.data.f1");
        }
        actions = {
            setb1();
            noop();
        }
        default_action = noop();
    }
    @name("ingress.ex1") table ex1_0 {
        key = {
            hdrs.extra[0].h: ternary @name("hdrs.extra[0].h");
        }
        actions = {
            setbyte();
            act1();
            act2();
            act3();
            noop_1();
        }
        default_action = noop_1();
    }
    @name("ingress.tbl1") table tbl1_0 {
        key = {
            hdrs.data.f2: ternary @name("hdrs.data.f2");
        }
        actions = {
            setbyte_1();
            noop_2();
        }
        default_action = noop_2();
    }
    @name("ingress.tbl2") table tbl2_0 {
        key = {
            hdrs.data.f2: ternary @name("hdrs.data.f2");
        }
        actions = {
            setbyte_2();
            noop_3();
        }
        default_action = noop_3();
    }
    @name("ingress.tbl3") table tbl3_0 {
        key = {
            hdrs.data.f2: ternary @name("hdrs.data.f2");
        }
        actions = {
            setbyte_3();
            noop_4();
        }
        default_action = noop_4();
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

control egress(inout packet_t hdrs, inout Meta m, inout standard_metadata_t meta) {
    apply {
    }
}

control deparser(packet_out b, in packet_t hdrs) {
    apply {
        b.emit<data_h>(hdrs.data);
        b.emit<extra_h>(hdrs.extra[0]);
        b.emit<extra_h>(hdrs.extra[1]);
        b.emit<extra_h>(hdrs.extra[2]);
        b.emit<extra_h>(hdrs.extra[3]);
    }
}

V1Switch<packet_t, Meta>(p(), vrfy(), ingress(), egress(), update(), deparser()) main;
