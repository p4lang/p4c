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
    @hidden action invalidhdrwarnings2l36() {
        h2_0 = h1_0;
    }
    @hidden action invalidhdrwarnings2l38() {
        h2_0.setValid();
        h2_0.data = 32w0;
    }
    @hidden action invalidhdrwarnings2l23() {
        h1_0.setInvalid();
        h2_0.setInvalid();
        s1_0_h1.setInvalid();
        s1_0_h2.setInvalid();
        s2_0_h1.setInvalid();
        s2_0_h2.setInvalid();
        h1_0.setValid();
        h1_0.data = 32w1;
    }
    @hidden action invalidhdrwarnings2l46() {
        h1_0.setInvalid();
    }
    @hidden action invalidhdrwarnings2l43() {
        hdr.h2.data = h2_0.data;
    }
    @hidden action invalidhdrwarnings2l57() {
        s1_0_h1.setInvalid();
    }
    @hidden action invalidhdrwarnings2l59() {
        s1_0_h1.setInvalid();
    }
    @hidden action invalidhdrwarnings2l60() {
        hdr.h1.data = s1_0_h1.data;
    }
    @hidden action invalidhdrwarnings2l62() {
        s1_0_h1.setValid();
    }
    @hidden action invalidhdrwarnings2l53() {
        s1_0_h1 = h2_0;
        s1_0_h2 = h2_0;
        s2_0_h1 = h2_0;
        s2_0_h2 = h2_0;
    }
    @hidden action invalidhdrwarnings2l64() {
        hdr.h2.data = s1_0_h1.data;
    }
    @hidden action invalidhdrwarnings2l49() {
        hdr.h1.data = h1_0.data;
        h1_0 = h2_0;
    }
    @hidden action invalidhdrwarnings2l69() {
        hdr.h1.data = 32w1;
        hdr.h2.data = 32w1;
    }
    @hidden table tbl_invalidhdrwarnings2l23 {
        actions = {
            invalidhdrwarnings2l23();
        }
        const default_action = invalidhdrwarnings2l23();
    }
    @hidden table tbl_invalidhdrwarnings2l36 {
        actions = {
            invalidhdrwarnings2l36();
        }
        const default_action = invalidhdrwarnings2l36();
    }
    @hidden table tbl_invalidhdrwarnings2l38 {
        actions = {
            invalidhdrwarnings2l38();
        }
        const default_action = invalidhdrwarnings2l38();
    }
    @hidden table tbl_invalidhdrwarnings2l43 {
        actions = {
            invalidhdrwarnings2l43();
        }
        const default_action = invalidhdrwarnings2l43();
    }
    @hidden table tbl_invalidhdrwarnings2l46 {
        actions = {
            invalidhdrwarnings2l46();
        }
        const default_action = invalidhdrwarnings2l46();
    }
    @hidden table tbl_invalidhdrwarnings2l49 {
        actions = {
            invalidhdrwarnings2l49();
        }
        const default_action = invalidhdrwarnings2l49();
    }
    @hidden table tbl_invalidhdrwarnings2l53 {
        actions = {
            invalidhdrwarnings2l53();
        }
        const default_action = invalidhdrwarnings2l53();
    }
    @hidden table tbl_invalidhdrwarnings2l57 {
        actions = {
            invalidhdrwarnings2l57();
        }
        const default_action = invalidhdrwarnings2l57();
    }
    @hidden table tbl_invalidhdrwarnings2l59 {
        actions = {
            invalidhdrwarnings2l59();
        }
        const default_action = invalidhdrwarnings2l59();
    }
    @hidden table tbl_invalidhdrwarnings2l60 {
        actions = {
            invalidhdrwarnings2l60();
        }
        const default_action = invalidhdrwarnings2l60();
    }
    @hidden table tbl_invalidhdrwarnings2l62 {
        actions = {
            invalidhdrwarnings2l62();
        }
        const default_action = invalidhdrwarnings2l62();
    }
    @hidden table tbl_invalidhdrwarnings2l64 {
        actions = {
            invalidhdrwarnings2l64();
        }
        const default_action = invalidhdrwarnings2l64();
    }
    @hidden table tbl_invalid_H {
        actions = {
            invalid_H();
        }
        const default_action = invalid_H();
    }
    @hidden table tbl_invalidhdrwarnings2l69 {
        actions = {
            invalidhdrwarnings2l69();
        }
        const default_action = invalidhdrwarnings2l69();
    }
    apply {
        tbl_invalidhdrwarnings2l23.apply();
        if (hdr.h1.data == 32w0) {
            tbl_invalidhdrwarnings2l36.apply();
        } else {
            tbl_invalidhdrwarnings2l38.apply();
        }
        tbl_invalidhdrwarnings2l43.apply();
        if (h2_0.data == 32w1) {
            tbl_invalidhdrwarnings2l46.apply();
        }
        tbl_invalidhdrwarnings2l49.apply();
        if (h1_0.data == 32w1) {
            tbl_invalidhdrwarnings2l53.apply();
            if (s2_0_h1.data == 32w0 && s2_0_h2.data == 32w0) {
                if (s1_0_h1.data > 32w0) {
                    tbl_invalidhdrwarnings2l57.apply();
                } else {
                    tbl_invalidhdrwarnings2l59.apply();
                }
                tbl_invalidhdrwarnings2l60.apply();
            } else {
                tbl_invalidhdrwarnings2l62.apply();
            }
            tbl_invalidhdrwarnings2l64.apply();
        }
        tbl_invalid_H.apply();
        tbl_invalidhdrwarnings2l69.apply();
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

V1Switch<H, M>(ParserI(), VerifyChecksumI(), IngressI(), EgressI(), ComputeChecksumI(), DeparserI()) main;

