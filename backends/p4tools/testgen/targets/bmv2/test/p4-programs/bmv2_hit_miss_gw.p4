#include <v1model.p4>

header data_t {
    bit<8>  c1;
    bit<8>  c2;
    bit<8>  c3;
    bit<32> r1;
    bit<32> r2;
    bit<32> r3;
    bit<8>  b1;
    bit<8>  b2;
    bit<8>  b3;
    bit<8>  b4;
    bit<8>  b5;
    bit<8>  b6;
}

struct Headers {
    data_t data;
}

struct Meta {}

parser p(packet_in pkt, out Headers h, inout Meta meta, inout standard_metadata_t stdmeta) {
    state start {
        pkt.extract(h.data);
        transition accept;
    }
}

control vrfy(inout Headers h, inout Meta meta) {
    apply { }
}

control ingress(inout Headers hdr, inout Meta m, inout standard_metadata_t s) {
    action noop() {}

    action setb1(bit<8> b1) {
        hdr.data.b1 = b1;
    }

    action setb2(bit<8> b2) {
        hdr.data.b2 = b2;
    }

    action setb3(bit<8> b3) {
        hdr.data.b3 = b3;
    }

    table t1 {
        key = { hdr.data.r1 : exact; }
        actions = { setb1; noop; }
        size = 1024;
        const default_action = noop;
    }

    table t2 {
        key = { hdr.data.r2 : exact; }
        actions = { setb2; noop; }
        size = 1024;
        const default_action = noop;
    }

    table t3 {
        key = { hdr.data.b1 : exact;
                hdr.data.b2 : exact; }
        actions = { setb3;  noop; }
        size = 1024;
    }

    apply {
        if (hdr.data.c1 == 0) {
            if (t1.apply().miss) {
                t2.apply();
            }
            t3.apply();
        }
    }
}

control egress(inout Headers h, inout Meta m, inout standard_metadata_t s) {
    apply { }
}

control update(inout Headers h, inout Meta m) {
    apply { }
}

control deparser(packet_out pkt, in Headers h) {
    apply {
        pkt.emit(h);
    }
}

V1Switch(p(), vrfy(), ingress(), egress(), update(), deparser()) main;
