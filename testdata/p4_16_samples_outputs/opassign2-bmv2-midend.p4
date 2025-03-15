#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

header data_t {
    bit<32> f1;
    bit<32> f2;
    bit<16> h1;
    bit<16> h2;
    bit<8>  b1;
    bit<8>  b2;
    bit<8>  b3;
    bit<8>  b4;
}

header word_t {
    bit<32> x;
}

struct metadata {
}

struct headers {
    data_t    data;
    word_t[8] rest;
}

parser p(packet_in b, out headers hdr, inout metadata meta, inout standard_metadata_t stdmeta) {
    state start {
        b.extract<data_t>(hdr.data);
        b.extract<word_t>(hdr.rest.next);
        b.extract<word_t>(hdr.rest.next);
        b.extract<word_t>(hdr.rest.next);
        b.extract<word_t>(hdr.rest.next);
        b.extract<word_t>(hdr.rest.next);
        b.extract<word_t>(hdr.rest.next);
        b.extract<word_t>(hdr.rest.next);
        b.extract<word_t>(hdr.rest.next);
        transition accept;
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t stdmeta) {
    bit<8> hsiVar;
    bit<8> hsiVar_0;
    @name("ingress.retval") bit<8> retval;
    @name("ingress.retval") bit<8> retval_1;
    @hidden action opassign2bmv2l55() {
        hdr.rest[8w0].x = hdr.rest[8w0].x | hdr.data.f1;
    }
    @hidden action opassign2bmv2l55_0() {
        hdr.rest[8w1].x = hdr.rest[8w1].x | hdr.data.f1;
    }
    @hidden action opassign2bmv2l55_1() {
        hdr.rest[8w2].x = hdr.rest[8w2].x | hdr.data.f1;
    }
    @hidden action opassign2bmv2l55_2() {
        hdr.rest[8w3].x = hdr.rest[8w3].x | hdr.data.f1;
    }
    @hidden action opassign2bmv2l55_3() {
        hdr.rest[8w4].x = hdr.rest[8w4].x | hdr.data.f1;
    }
    @hidden action opassign2bmv2l55_4() {
        hdr.rest[8w5].x = hdr.rest[8w5].x | hdr.data.f1;
    }
    @hidden action opassign2bmv2l55_5() {
        hdr.rest[8w6].x = hdr.rest[8w6].x | hdr.data.f1;
    }
    @hidden action opassign2bmv2l55_6() {
        hdr.rest[8w7].x = hdr.rest[8w7].x | hdr.data.f1;
    }
    @hidden action opassign2bmv2l47() {
        retval = hdr.data.b1;
        hdr.data.b1 = hdr.data.b1 + 8w1;
        hsiVar = retval & 8w7;
    }
    @hidden action opassign2bmv2l56() {
        hdr.rest[8w0].x = hdr.rest[8w0].x | hdr.data.f2;
    }
    @hidden action opassign2bmv2l56_0() {
        hdr.rest[8w1].x = hdr.rest[8w1].x | hdr.data.f2;
    }
    @hidden action opassign2bmv2l56_1() {
        hdr.rest[8w2].x = hdr.rest[8w2].x | hdr.data.f2;
    }
    @hidden action opassign2bmv2l56_2() {
        hdr.rest[8w3].x = hdr.rest[8w3].x | hdr.data.f2;
    }
    @hidden action opassign2bmv2l56_3() {
        hdr.rest[8w4].x = hdr.rest[8w4].x | hdr.data.f2;
    }
    @hidden action opassign2bmv2l56_4() {
        hdr.rest[8w5].x = hdr.rest[8w5].x | hdr.data.f2;
    }
    @hidden action opassign2bmv2l56_5() {
        hdr.rest[8w6].x = hdr.rest[8w6].x | hdr.data.f2;
    }
    @hidden action opassign2bmv2l56_6() {
        hdr.rest[8w7].x = hdr.rest[8w7].x | hdr.data.f2;
    }
    @hidden action opassign2bmv2l47_0() {
        retval_1 = hdr.data.b1;
        hdr.data.b1 = hdr.data.b1 + 8w1;
        hsiVar_0 = retval_1 & 8w7;
    }
    @hidden table tbl_opassign2bmv2l47 {
        actions = {
            opassign2bmv2l47();
        }
        const default_action = opassign2bmv2l47();
    }
    @hidden table tbl_opassign2bmv2l55 {
        actions = {
            opassign2bmv2l55();
        }
        const default_action = opassign2bmv2l55();
    }
    @hidden table tbl_opassign2bmv2l55_0 {
        actions = {
            opassign2bmv2l55_0();
        }
        const default_action = opassign2bmv2l55_0();
    }
    @hidden table tbl_opassign2bmv2l55_1 {
        actions = {
            opassign2bmv2l55_1();
        }
        const default_action = opassign2bmv2l55_1();
    }
    @hidden table tbl_opassign2bmv2l55_2 {
        actions = {
            opassign2bmv2l55_2();
        }
        const default_action = opassign2bmv2l55_2();
    }
    @hidden table tbl_opassign2bmv2l55_3 {
        actions = {
            opassign2bmv2l55_3();
        }
        const default_action = opassign2bmv2l55_3();
    }
    @hidden table tbl_opassign2bmv2l55_4 {
        actions = {
            opassign2bmv2l55_4();
        }
        const default_action = opassign2bmv2l55_4();
    }
    @hidden table tbl_opassign2bmv2l55_5 {
        actions = {
            opassign2bmv2l55_5();
        }
        const default_action = opassign2bmv2l55_5();
    }
    @hidden table tbl_opassign2bmv2l55_6 {
        actions = {
            opassign2bmv2l55_6();
        }
        const default_action = opassign2bmv2l55_6();
    }
    @hidden table tbl_opassign2bmv2l47_0 {
        actions = {
            opassign2bmv2l47_0();
        }
        const default_action = opassign2bmv2l47_0();
    }
    @hidden table tbl_opassign2bmv2l56 {
        actions = {
            opassign2bmv2l56();
        }
        const default_action = opassign2bmv2l56();
    }
    @hidden table tbl_opassign2bmv2l56_0 {
        actions = {
            opassign2bmv2l56_0();
        }
        const default_action = opassign2bmv2l56_0();
    }
    @hidden table tbl_opassign2bmv2l56_1 {
        actions = {
            opassign2bmv2l56_1();
        }
        const default_action = opassign2bmv2l56_1();
    }
    @hidden table tbl_opassign2bmv2l56_2 {
        actions = {
            opassign2bmv2l56_2();
        }
        const default_action = opassign2bmv2l56_2();
    }
    @hidden table tbl_opassign2bmv2l56_3 {
        actions = {
            opassign2bmv2l56_3();
        }
        const default_action = opassign2bmv2l56_3();
    }
    @hidden table tbl_opassign2bmv2l56_4 {
        actions = {
            opassign2bmv2l56_4();
        }
        const default_action = opassign2bmv2l56_4();
    }
    @hidden table tbl_opassign2bmv2l56_5 {
        actions = {
            opassign2bmv2l56_5();
        }
        const default_action = opassign2bmv2l56_5();
    }
    @hidden table tbl_opassign2bmv2l56_6 {
        actions = {
            opassign2bmv2l56_6();
        }
        const default_action = opassign2bmv2l56_6();
    }
    apply {
        if (hdr.rest[7].isValid()) {
            tbl_opassign2bmv2l47.apply();
            if (hsiVar == 8w0) {
                tbl_opassign2bmv2l55.apply();
            } else if (hsiVar == 8w1) {
                tbl_opassign2bmv2l55_0.apply();
            } else if (hsiVar == 8w2) {
                tbl_opassign2bmv2l55_1.apply();
            } else if (hsiVar == 8w3) {
                tbl_opassign2bmv2l55_2.apply();
            } else if (hsiVar == 8w4) {
                tbl_opassign2bmv2l55_3.apply();
            } else if (hsiVar == 8w5) {
                tbl_opassign2bmv2l55_4.apply();
            } else if (hsiVar == 8w6) {
                tbl_opassign2bmv2l55_5.apply();
            } else if (hsiVar == 8w7) {
                tbl_opassign2bmv2l55_6.apply();
            }
            tbl_opassign2bmv2l47_0.apply();
            if (hsiVar_0 == 8w0) {
                tbl_opassign2bmv2l56.apply();
            } else if (hsiVar_0 == 8w1) {
                tbl_opassign2bmv2l56_0.apply();
            } else if (hsiVar_0 == 8w2) {
                tbl_opassign2bmv2l56_1.apply();
            } else if (hsiVar_0 == 8w3) {
                tbl_opassign2bmv2l56_2.apply();
            } else if (hsiVar_0 == 8w4) {
                tbl_opassign2bmv2l56_3.apply();
            } else if (hsiVar_0 == 8w5) {
                tbl_opassign2bmv2l56_4.apply();
            } else if (hsiVar_0 == 8w6) {
                tbl_opassign2bmv2l56_5.apply();
            } else if (hsiVar_0 == 8w7) {
                tbl_opassign2bmv2l56_6.apply();
            }
        }
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t stdmeta) {
    apply {
    }
}

control vc(inout headers hdr, inout metadata meta) {
    apply {
    }
}

control uc(inout headers hdr, inout metadata meta) {
    apply {
    }
}

control deparser(packet_out packet, in headers hdr) {
    apply {
        packet.emit<data_t>(hdr.data);
        packet.emit<word_t>(hdr.rest[0]);
        packet.emit<word_t>(hdr.rest[1]);
        packet.emit<word_t>(hdr.rest[2]);
        packet.emit<word_t>(hdr.rest[3]);
        packet.emit<word_t>(hdr.rest[4]);
        packet.emit<word_t>(hdr.rest[5]);
        packet.emit<word_t>(hdr.rest[6]);
        packet.emit<word_t>(hdr.rest[7]);
    }
}

V1Switch<headers, metadata>(p(), vc(), ingress(), egress(), uc(), deparser()) main;
