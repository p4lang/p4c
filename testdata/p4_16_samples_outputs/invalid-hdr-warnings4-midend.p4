#include <core.p4>
#define V1MODEL_VERSION 20200408
#include <v1model.p4>

header Header {
    bit<32> data;
}

struct H {
    Header[2] h1;
    Header[2] h2;
}

struct M {
}

parser ParserI(packet_in pkt, out H hdr, inout M meta, inout standard_metadata_t smeta) {
    state start {
        pkt.extract<Header>(hdr.h1[0]);
        transition init;
    }
    state init {
        pkt.extract<Header>(hdr.h1[1]);
        hdr.h1[0].data = 32w1;
        hdr.h1[1].data = 32w1;
        hdr.h1[0].setInvalid();
        pkt.extract<Header>(hdr.h1.next);
        hdr.h1[0].data = 32w1;
        hdr.h1[1].data = 32w1;
        hdr.h1[0].setInvalid();
        transition accept;
    }
}

control IngressI(inout H hdr, inout M meta, inout standard_metadata_t smeta) {
    Header[2] h_0_h1;
    Header[2] h_0_h2;
    Header[2] h_copy_0_h1;
    Header[2] h_copy_0_h2;
    Header[2] h_copy2_0_h1;
    Header[2] h_copy2_0_h2;
    @name("IngressI.hd") Header hd_0;
    @name("IngressI.hd") Header hd_1;
    @name("IngressI.hd") Header hd_8;
    @name("IngressI.hd") Header hd_9;
    @name("IngressI.hd") Header hd_10;
    @name("IngressI.hd") Header hd_11;
    @name("IngressI.hd") Header hd_12;
    @name("IngressI.validateHeader") action validateHeader() {
        hd_10.setValid();
    }
    @name("IngressI.validateHeader") action validateHeader_1() {
        hd_11.setValid();
    }
    @name("IngressI.invalidateHeader") action invalidateHeader() {
        hd_12.setInvalid();
    }
    @name("IngressI.invalidateStack") action invalidateStack() {
        hd_0.setInvalid();
        hd_1.setInvalid();
    }
    @name("IngressI.invalidateStack") action invalidateStack_1() {
        hd_8.setInvalid();
        hd_9.setInvalid();
    }
    @hidden action invalidhdrwarnings4l45() {
        h_0_h1[0].setInvalid();
        h_0_h1[1].setInvalid();
        h_0_h2[0].setInvalid();
        h_0_h2[1].setInvalid();
        h_copy_0_h1[0].setInvalid();
        h_copy_0_h1[1].setInvalid();
        h_copy_0_h2[0].setInvalid();
        h_copy_0_h2[1].setInvalid();
        h_copy2_0_h1[0].setInvalid();
        h_copy2_0_h1[1].setInvalid();
        h_copy2_0_h2[0].setInvalid();
        h_copy2_0_h2[1].setInvalid();
    }
    @hidden action invalidhdrwarnings4l108() {
        h_0_h1[1].setValid();
    }
    @hidden table tbl_invalidhdrwarnings4l45 {
        actions = {
            invalidhdrwarnings4l45();
        }
        const default_action = invalidhdrwarnings4l45();
    }
    @hidden table tbl_validateHeader {
        actions = {
            validateHeader();
        }
        const default_action = validateHeader();
    }
    @hidden table tbl_invalidateHeader {
        actions = {
            invalidateHeader();
        }
        const default_action = invalidateHeader();
    }
    @hidden table tbl_validateHeader_0 {
        actions = {
            validateHeader_1();
        }
        const default_action = validateHeader_1();
    }
    @hidden table tbl_invalidateStack {
        actions = {
            invalidateStack();
        }
        const default_action = invalidateStack();
    }
    @hidden table tbl_invalidateStack_0 {
        actions = {
            invalidateStack_1();
        }
        const default_action = invalidateStack_1();
    }
    @hidden table tbl_invalidhdrwarnings4l108 {
        actions = {
            invalidhdrwarnings4l108();
        }
        const default_action = invalidhdrwarnings4l108();
    }
    apply {
        tbl_invalidhdrwarnings4l45.apply();
        tbl_validateHeader.apply();
        tbl_invalidateHeader.apply();
        tbl_validateHeader_0.apply();
        tbl_invalidateStack.apply();
        tbl_invalidateStack_0.apply();
        tbl_invalidhdrwarnings4l108.apply();
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
