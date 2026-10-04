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
        pkt.extract < Header >(hdr.h1);
        transition accept;
    }
}

control IngressI(inout H hdr, inout M meta, inout standard_metadata_t smeta) {
    @name("IngressI.h1") Header h1_0;
    @name("IngressI.h2") Header h2_0;
    @hidden action switch_0_case() {
    }
    @hidden action switch_0_case_0() {
    }
    @hidden table switch_0_table {
        key = {
            hdr.h1.data : exact;
        }
        actions = {
            switch_0_case();
            switch_0_case_0();
        }
        const default_action = switch_0_case_0();
        const entries = {
            const 32w0 : switch_0_case();
        }
    }
    @hidden action switch_1_case() {
    }
    @hidden action switch_1_case_0() {
    }
    @hidden table switch_1_table {
        key = {
            h2_0.data : exact;
        }
        actions = {
            switch_1_case();
            switch_1_case_0();
        }
        const default_action = switch_1_case_0();
        const entries = {
            const 32w0 : switch_1_case();
        }
    }
    @hidden action invalidhdrwarnings3bmv2l36() {
        h1_0.setValid();
        h2_0.setInvalid();
    }
    @hidden action invalidhdrwarnings3bmv2l38() {
        h1_0.setValid();
        h2_0.setInvalid();
    }
    @hidden action invalidhdrwarnings3bmv2l25() {
        h1_0.setInvalid();
        h2_0.setInvalid();
        h1_0.setInvalid();
        h2_0.setValid();
        h2_0.data = 32w1;
    }
    @hidden action invalidhdrwarnings3bmv2l47() {
        h1_0.setValid();
        h2_0.setValid();
    }
    @hidden action invalidhdrwarnings3bmv2l49() {
        h1_0.setInvalid();
        h2_0.setInvalid();
    }
    @hidden action invalidhdrwarnings3bmv2l43() {
        hdr.h1.data = h2_0.data;
    }
    @hidden action invalidhdrwarnings3bmv2l54() {
        hdr.h1.data = h2_0.data;
    }
    @hidden table tbl_invalidhdrwarnings3bmv2l25 {
        actions = {
            invalidhdrwarnings3bmv2l25();
        }
        const default_action = invalidhdrwarnings3bmv2l25();
    }
    @hidden table tbl_invalidhdrwarnings3bmv2l36 {
        actions = {
            invalidhdrwarnings3bmv2l36();
        }
        const default_action = invalidhdrwarnings3bmv2l36();
    }
    @hidden table tbl_invalidhdrwarnings3bmv2l38 {
        actions = {
            invalidhdrwarnings3bmv2l38();
        }
        const default_action = invalidhdrwarnings3bmv2l38();
    }
    @hidden table tbl_invalidhdrwarnings3bmv2l43 {
        actions = {
            invalidhdrwarnings3bmv2l43();
        }
        const default_action = invalidhdrwarnings3bmv2l43();
    }
    @hidden table tbl_invalidhdrwarnings3bmv2l47 {
        actions = {
            invalidhdrwarnings3bmv2l47();
        }
        const default_action = invalidhdrwarnings3bmv2l47();
    }
    @hidden table tbl_invalidhdrwarnings3bmv2l49 {
        actions = {
            invalidhdrwarnings3bmv2l49();
        }
        const default_action = invalidhdrwarnings3bmv2l49();
    }
    @hidden table tbl_invalidhdrwarnings3bmv2l54 {
        actions = {
            invalidhdrwarnings3bmv2l54();
        }
        const default_action = invalidhdrwarnings3bmv2l54();
    }
    apply {
        tbl_invalidhdrwarnings3bmv2l25.apply();
        switch (switch_0_table.apply().action_run) {
            switch_0_case : {
            tbl_invalidhdrwarnings3bmv2l36.apply();
            }
            switch_0_case_0 : {
            tbl_invalidhdrwarnings3bmv2l38.apply();
            }
        }
        tbl_invalidhdrwarnings3bmv2l43.apply();
        switch (switch_1_table.apply().action_run) {
            switch_1_case : {
            tbl_invalidhdrwarnings3bmv2l47.apply();
            }
            switch_1_case_0 : {
            tbl_invalidhdrwarnings3bmv2l49.apply();
            }
        }
        tbl_invalidhdrwarnings3bmv2l54.apply();
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
