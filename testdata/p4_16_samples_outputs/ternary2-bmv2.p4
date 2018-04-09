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
        b.extract(hdrs.data);
        transition extra;
    }
    state extra {
        b.extract(hdrs.extra.next);
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
    action setb1(bit<9> port, bit<8> val) {
        hdrs.data.b1 = val;
        meta.egress_spec = port;
    }
    action noop() {
    }
    action setbyte(out bit<8> reg, bit<8> val) {
        reg = val;
    }
    action act1(bit<8> val) {
        hdrs.extra[0].b1 = val;
    }
    action act2(bit<8> val) {
        hdrs.extra[0].b1 = val;
    }
    action act3(bit<8> val) {
        hdrs.extra[0].b1 = val;
    }
    table test1 {
        key = {
            hdrs.data.f1: ternary;
        }
        actions = {
            setb1;
            noop;
        }
        default_action = noop;
    }
    table ex1 {
        key = {
            hdrs.extra[0].h: ternary;
        }
        actions = {
            setbyte(hdrs.extra[0].b1);
            act1;
            act2;
            act3;
            noop;
        }
        default_action = noop;
    }
    table tbl1 {
        key = {
            hdrs.data.f2: ternary;
        }
        actions = {
            setbyte(hdrs.data.b2);
            noop;
        }
        default_action = noop;
    }
    table tbl2 {
        key = {
            hdrs.data.f2: ternary;
        }
        actions = {
            setbyte(hdrs.extra[1].b1);
            noop;
        }
        default_action = noop;
    }
    table tbl3 {
        key = {
            hdrs.data.f2: ternary;
        }
        actions = {
            setbyte(hdrs.extra[2].b2);
            noop;
        }
        default_action = noop;
    }
    apply {
        test1.apply();
        switch (ex1.apply().action_run) {
            act1: {
                tbl1.apply();
            }
            act2: {
                tbl2.apply();
            }
            act3: {
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
        b.emit(hdrs.data);
        b.emit(hdrs.extra);
    }
}

V1Switch(p(), vrfy(), ingress(), egress(), update(), deparser()) main;

