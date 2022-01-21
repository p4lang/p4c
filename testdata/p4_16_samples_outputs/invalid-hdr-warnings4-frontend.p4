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
        transition select(hdr.h1[1].data) {
            32w0: init;
            default: accept;
        }
    }
}

control IngressI(inout H hdr, inout M meta, inout standard_metadata_t smeta) {
    @name("IngressI.h") H h_0;
    @name("IngressI.h_copy") H h_copy_0;
    @name("IngressI.h_copy2") H h_copy2_0;
    @name("IngressI.i") bit<1> i_0;
    @name("IngressI.tmp") bit<1> tmp;
    @name("IngressI.hd") Header hd_0;
    @name("IngressI.hd") Header hd_1;
    @name("IngressI.hd") Header hd_8;
    @name("IngressI.hd") Header hd_9;
    @name("IngressI.hd") Header hd_10;
    @name("IngressI.hd") Header hd_11;
    @name("IngressI.hd") Header hd_12;
    @name("IngressI.stack") Header[2] stack_0;
    @name("IngressI.stack") Header[2] stack_2;
    @name("IngressI.validateHeader") action validateHeader() {
        hd_10 = h_0.h1[0];
        hd_10.setValid();
        h_0.h1[0] = hd_10;
    }
    @name("IngressI.validateHeader") action validateHeader_1() {
        hd_11 = h_0.h2[tmp];
        hd_11.setValid();
        h_0.h2[tmp] = hd_11;
    }
    @name("IngressI.invalidateHeader") action invalidateHeader() {
        hd_12 = h_0.h2[0];
        hd_12.setInvalid();
        h_0.h2[0] = hd_12;
    }
    @name("IngressI.invalidateStack") action invalidateStack() {
        stack_0 = h_0.h1;
        hd_0 = stack_0[0];
        hd_0.setInvalid();
        stack_0[0] = hd_0;
        hd_1 = stack_0[1];
        hd_1.setInvalid();
        stack_0[1] = hd_1;
        h_0.h1 = stack_0;
    }
    @name("IngressI.invalidateStack") action invalidateStack_1() {
        stack_2 = h_copy_0.h1;
        hd_8 = stack_2[0];
        hd_8.setInvalid();
        stack_2[0] = hd_8;
        hd_9 = stack_2[1];
        hd_9.setInvalid();
        stack_2[1] = hd_9;
        h_copy_0.h1 = stack_2;
    }
    apply {
        h_0.h1[0].setInvalid();
        h_0.h1[1].setInvalid();
        h_0.h2[0].setInvalid();
        h_0.h2[1].setInvalid();
        h_copy_0.h1[0].setInvalid();
        h_copy_0.h1[1].setInvalid();
        h_copy_0.h2[0].setInvalid();
        h_copy_0.h2[1].setInvalid();
        h_copy2_0.h1[0].setInvalid();
        h_copy2_0.h1[1].setInvalid();
        h_copy2_0.h2[0].setInvalid();
        h_copy2_0.h2[1].setInvalid();
        validateHeader();
        h_0.h1[0].data = 32w1;
        h_0.h1[1] = h_0.h1[0];
        h_0.h1[1].data = 32w1;
        h_0.h2 = h_0.h1;
        h_0.h2[0].data = 32w1;
        h_0.h2[1].data = 32w1;
        h_copy_0 = h_0;
        h_copy_0.h1[0].data = 32w1;
        h_copy_0.h1[1].data = 32w1;
        invalidateHeader();
        i_0 = 1w1;
        tmp = i_0;
        validateHeader_1();
        invalidateStack();
        invalidateStack_1();
        h_0.h1[1].setValid();
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

