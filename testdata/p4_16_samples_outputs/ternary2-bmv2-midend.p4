#include <core.p4>
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
    @name("ingress.setb1") action setb1_0(bit<9> port, bit<8> val) {
        hdrs.data.b1 = val;
        meta.egress_spec = port;
    }
    @name("ingress.noop") action noop_0() {
    }
    @name("ingress.noop") action noop_5() {
    }
    @name("ingress.noop") action noop_6() {
    }
    @name("ingress.noop") action noop_7() {
    }
    @name("ingress.noop") action noop_8() {
    }
    @name("ingress.setbyte") action setbyte_0(bit<8> val) {
        hdrs.extra[0].b1 = val;
    }
    @name("ingress.setbyte") action setbyte_4(bit<8> val) {
        hdrs.data.b2 = val;
    }
    @name("ingress.setbyte") action setbyte_5(bit<8> val) {
        hdrs.extra[1].b1 = val;
    }
    @name("ingress.setbyte") action setbyte_6(bit<8> val) {
        hdrs.extra[2].b2 = val;
    }
    @name("ingress.act1") action act1_0(bit<8> val) {
        hdrs.extra[0].b1 = val;
    }
    @name("ingress.act2") action act2_0(bit<8> val) {
        hdrs.extra[0].b1 = val;
    }
    @name("ingress.act3") action act3_0(bit<8> val) {
        hdrs.extra[0].b1 = val;
    }
    @name("ingress.test1") table test1 {
        key = {
            hdrs.data.f1: ternary @name("hdrs.data.f1") ;
        }
        actions = {
            setb1_0();
            noop_0();
        }
        default_action = noop_0();
    }
    @name("ingress.ex1") table ex1 {
        key = {
            hdrs.extra[0].h: ternary @name("hdrs.extra[0].h") ;
        }
        actions = {
            setbyte_0();
            act1_0();
            act2_0();
            act3_0();
            noop_5();
        }
        default_action = noop_5();
    }
    @name("ingress.tbl1") table tbl1 {
        key = {
            hdrs.data.f2: ternary @name("hdrs.data.f2") ;
        }
        actions = {
            setbyte_4();
            noop_6();
        }
        default_action = noop_6();
    }
    @name("ingress.tbl2") table tbl2 {
        key = {
            hdrs.data.f2: ternary @name("hdrs.data.f2") ;
        }
        actions = {
            setbyte_5();
            noop_7();
        }
        default_action = noop_7();
    }
    @name("ingress.tbl3") table tbl3 {
        key = {
            hdrs.data.f2: ternary @name("hdrs.data.f2") ;
        }
        actions = {
            setbyte_6();
            noop_8();
        }
        default_action = noop_8();
    }
    apply {
        test1.apply();
        switch (ex1.apply().action_run) {
            act1_0: {
                tbl1.apply();
            }
            act2_0: {
                tbl2.apply();
            }
            act3_0: {
                tbl3.apply();
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

