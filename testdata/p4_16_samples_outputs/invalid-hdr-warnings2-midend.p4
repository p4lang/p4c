#include <core.p4>
#define V1MODEL_VERSION 20200408
#include <v1model.p4>

header Header {
    bit<32> data;
}

struct H {
    Header h1;
    Header h2;
}

struct M {
}

parser ParserI(packet_in pkt, out H hdr, inout M meta, inout standard_metadata_t smeta) {
    state start {
        transition accept;
    }
}

control IngressI(inout H hdr, inout M meta, inout standard_metadata_t smeta) {
    @name("IngressI.h1") Header h1_0;
    @name("IngressI.h2") Header h2_0;
    Header s1_0_h1;
    Header s1_0_h2;
    Header s2_0_h1;
    Header s2_0_h2;
    Header s_0_h1;
    Header s_0_h2;
    @name("IngressI.invalid_H") action invalid_H() {
        s_0_h1 = hdr.h1;
        s_0_h2 = hdr.h2;
        s_0_h1.setInvalid();
        s_0_h2.setInvalid();
        hdr.h1 = s_0_h1;
        hdr.h2 = s_0_h2;
    }
    @hidden action invalidhdrwarnings2l37() {
        h2_0 = h1_0;
    }
    @hidden action invalidhdrwarnings2l39() {
        h2_0.setValid();
        h2_0.data = 32w0;
    }
    @hidden action invalidhdrwarnings2l24() {
        h1_0.setInvalid();
        h2_0.setInvalid();
        s1_0_h1.setInvalid();
        s1_0_h2.setInvalid();
        s2_0_h1.setInvalid();
        s2_0_h2.setInvalid();
        h1_0.setValid();
        h1_0.data = 32w1;
    }
    @hidden action invalidhdrwarnings2l47() {
        h1_0.setInvalid();
    }
    @hidden action invalidhdrwarnings2l44() {
        hdr.h2.data = h2_0.data;
    }
    @hidden action invalidhdrwarnings2l58() {
        s1_0_h1.setInvalid();
    }
    @hidden action invalidhdrwarnings2l60() {
        s1_0_h1.setInvalid();
    }
    @hidden action invalidhdrwarnings2l61() {
        hdr.h1.data = s1_0_h1.data;
    }
    @hidden action invalidhdrwarnings2l63() {
        s1_0_h1.setValid();
    }
    @hidden action invalidhdrwarnings2l54() {
        s1_0_h1 = h2_0;
        s1_0_h2 = h2_0;
        s2_0_h1 = h2_0;
        s2_0_h2 = h2_0;
    }
    @hidden action invalidhdrwarnings2l65() {
        hdr.h2.data = s1_0_h1.data;
    }
    @hidden action invalidhdrwarnings2l50() {
        hdr.h1.data = h1_0.data;
        h1_0 = h2_0;
    }
    @hidden action invalidhdrwarnings2l70() {
        hdr.h1.data = 32w1;
        hdr.h2.data = 32w1;
    }
    @hidden table tbl_invalidhdrwarnings2l24 {
        actions = {
            invalidhdrwarnings2l24();
        }
        const default_action = invalidhdrwarnings2l24();
    }
    @hidden table tbl_invalidhdrwarnings2l37 {
        actions = {
            invalidhdrwarnings2l37();
        }
        const default_action = invalidhdrwarnings2l37();
    }
    @hidden table tbl_invalidhdrwarnings2l39 {
        actions = {
            invalidhdrwarnings2l39();
        }
        const default_action = invalidhdrwarnings2l39();
    }
    @hidden table tbl_invalidhdrwarnings2l44 {
        actions = {
            invalidhdrwarnings2l44();
        }
        const default_action = invalidhdrwarnings2l44();
    }
    @hidden table tbl_invalidhdrwarnings2l47 {
        actions = {
            invalidhdrwarnings2l47();
        }
        const default_action = invalidhdrwarnings2l47();
    }
    @hidden table tbl_invalidhdrwarnings2l50 {
        actions = {
            invalidhdrwarnings2l50();
        }
        const default_action = invalidhdrwarnings2l50();
    }
    @hidden table tbl_invalidhdrwarnings2l54 {
        actions = {
            invalidhdrwarnings2l54();
        }
        const default_action = invalidhdrwarnings2l54();
    }
    @hidden table tbl_invalidhdrwarnings2l58 {
        actions = {
            invalidhdrwarnings2l58();
        }
        const default_action = invalidhdrwarnings2l58();
    }
    @hidden table tbl_invalidhdrwarnings2l60 {
        actions = {
            invalidhdrwarnings2l60();
        }
        const default_action = invalidhdrwarnings2l60();
    }
    @hidden table tbl_invalidhdrwarnings2l61 {
        actions = {
            invalidhdrwarnings2l61();
        }
        const default_action = invalidhdrwarnings2l61();
    }
    @hidden table tbl_invalidhdrwarnings2l63 {
        actions = {
            invalidhdrwarnings2l63();
        }
        const default_action = invalidhdrwarnings2l63();
    }
    @hidden table tbl_invalidhdrwarnings2l65 {
        actions = {
            invalidhdrwarnings2l65();
        }
        const default_action = invalidhdrwarnings2l65();
    }
    @hidden table tbl_invalid_H {
        actions = {
            invalid_H();
        }
        const default_action = invalid_H();
    }
    @hidden table tbl_invalidhdrwarnings2l70 {
        actions = {
            invalidhdrwarnings2l70();
        }
        const default_action = invalidhdrwarnings2l70();
    }
    apply {
        tbl_invalidhdrwarnings2l24.apply();
        if (hdr.h1.data == 32w0) {
            tbl_invalidhdrwarnings2l37.apply();
        } else {
            tbl_invalidhdrwarnings2l39.apply();
        }
        tbl_invalidhdrwarnings2l44.apply();
        if (h2_0.data == 32w1) {
            tbl_invalidhdrwarnings2l47.apply();
        }
        tbl_invalidhdrwarnings2l50.apply();
        if (h1_0.data == 32w1) {
            tbl_invalidhdrwarnings2l54.apply();
            if (s2_0_h1.data == 32w0 && s2_0_h2.data == 32w0) {
                if (s1_0_h1.data > 32w0) {
                    tbl_invalidhdrwarnings2l58.apply();
                } else {
                    tbl_invalidhdrwarnings2l60.apply();
                }
                tbl_invalidhdrwarnings2l61.apply();
            } else {
                tbl_invalidhdrwarnings2l63.apply();
            }
            tbl_invalidhdrwarnings2l65.apply();
        }
        tbl_invalid_H.apply();
        tbl_invalidhdrwarnings2l70.apply();
    }
}

control EgressI(inout H hdr, inout M meta, inout standard_metadata_t smeta) {
    apply {
    }
}

control DeparserI(packet_out pk, in H hdr) {
    apply {
    }
}

control VerifyChecksumI(inout H hdr, inout M meta) {
    apply {
    }
}

control ComputeChecksumI(inout H hdr, inout M meta) {
    apply {
    }
}

V1Switch < H, M >(ParserI(), VerifyChecksumI(), IngressI(), EgressI(), ComputeChecksumI(), DeparserI()) main;
