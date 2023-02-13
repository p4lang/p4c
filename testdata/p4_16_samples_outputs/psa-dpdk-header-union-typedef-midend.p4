#include <core.p4>
#include <bmv2/psa.p4>

header EMPTY_H {
}

struct EMPTY_RESUB {
}

struct EMPTY_BRIDGE {
}

struct EMPTY_RECIRC {
}

header clone_0_t {
    bit<16> data;
}

header clone_1_t {
    bit<32> data;
}

struct clone_metadata_t {
    bit<3>    type;
    clone_0_t data_h0;
    clone_1_t data_h1;
}

header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
}

struct metadata {
    bit<32> meta;
    bit<32> meta1;
    bit<16> meta2;
    bit<32> meta3;
    bit<32> meta4;
    bit<16> meta5;
    bit<32> meta6;
    bit<16> meta7;
}

parser MyIP(packet_in buffer, out ethernet_t h, inout metadata b, in psa_ingress_parser_input_metadata_t c, in EMPTY_RESUB d, in EMPTY_RECIRC e) {
    state start {
        buffer.extract<ethernet_t>(h);
        transition accept;
    }
}

parser MyEP(packet_in buffer, out EMPTY_H a, inout metadata b, in psa_egress_parser_input_metadata_t c, in EMPTY_BRIDGE d, in clone_metadata_t e, in clone_metadata_t f) {
    @name("clone_md_data_h0") clone_0_t clone_md_data_h0_0;
    @name("clone_md_data_h1") clone_1_t clone_md_data_h1_0;
    state start {
        clone_md_data_h0_0.setInvalid();
        clone_md_data_h1_0.setInvalid();
        clone_md_data_h1_0.setValid();
        clone_md_data_h0_0.setInvalid();
        clone_md_data_h1_0.setValid();
        clone_md_data_h1_0.data = 32w0;
        clone_md_data_h0_0.setInvalid();
        transition select((bit<1>)(b.meta == 32w1)) {
            1w1: start_true;
            1w0: start_join;
            default: noMatch;
        }
    }
    state start_true {
        transition select(e.data_h0.isValid()) {
            true: start_true_true;
            false: start_true_false;
        }
    }
    state start_true_true {
        clone_md_data_h0_0.setValid();
        clone_md_data_h0_0 = e.data_h0;
        clone_md_data_h1_0.setInvalid();
        transition start_true_join;
    }
    state start_true_false {
        clone_md_data_h0_0.setInvalid();
        transition start_true_join;
    }
    state start_true_join {
        transition select(e.data_h1.isValid()) {
            true: start_true_true_0;
            false: start_true_false_0;
        }
    }
    state start_true_true_0 {
        clone_md_data_h1_0.setValid();
        clone_md_data_h1_0 = e.data_h1;
        clone_md_data_h0_0.setInvalid();
        transition start_true_join_0;
    }
    state start_true_false_0 {
        clone_md_data_h1_0.setInvalid();
        transition start_true_join_0;
    }
    state start_true_join_0 {
        transition start_join;
    }
    state start_join {
        b.meta1 = clone_md_data_h1_0.data;
        transition accept;
    }
    state noMatch {
        verify(false, error.NoMatch);
        transition reject;
    }
}

control MyIC(inout ethernet_t a, inout metadata b, in psa_ingress_input_metadata_t c, inout psa_ingress_output_metadata_t d) {
    @noWarn("unused") @name(".NoAction") action NoAction_0() {
    }
    @name("MyIC.forward") action forward() {
        b.meta = 32w0x1 << c.ingress_port;
    }
    @name("MyIC.tbl") table tbl_0 {
        key = {
            a.srcAddr: exact @name("a.srcAddr");
        }
        actions = {
            NoAction_0();
            forward();
        }
        default_action = NoAction_0();
    }
    apply {
        tbl_0.apply();
    }
}

control MyEC(inout EMPTY_H a, inout metadata b, in psa_egress_input_metadata_t c, inout psa_egress_output_metadata_t d) {
    apply {
    }
}

control MyID(packet_out buffer, out clone_metadata_t a, out EMPTY_RESUB b, out EMPTY_BRIDGE c, inout ethernet_t d, in metadata e, in psa_ingress_output_metadata_t f) {
    @hidden @name("psadpdkheaderuniontypedef125") action psadpdkheaderuniontypedef125_0() {
        buffer.emit<ethernet_t>(d);
    }
    @hidden @name("tbl_psadpdkheaderuniontypedef125") table tbl_psadpdkheaderuniontypedef125_0 {
        actions = {
            psadpdkheaderuniontypedef125_0();
        }
        const default_action = psadpdkheaderuniontypedef125_0();
    }
    apply {
        tbl_psadpdkheaderuniontypedef125_0.apply();
    }
}

control MyED(packet_out buffer, out clone_metadata_t a, out EMPTY_RECIRC b, inout EMPTY_H c, in metadata d, in psa_egress_output_metadata_t e, in psa_egress_deparser_input_metadata_t f) {
    apply {
    }
}

IngressPipeline<ethernet_t, metadata, EMPTY_BRIDGE, clone_metadata_t, EMPTY_RESUB, EMPTY_RECIRC>(MyIP(), MyIC(), MyID()) ip;
EgressPipeline<EMPTY_H, metadata, EMPTY_BRIDGE, clone_metadata_t, clone_metadata_t, EMPTY_RECIRC>(MyEP(), MyEC(), MyED()) ep;
PSA_Switch<ethernet_t, metadata, EMPTY_H, metadata, EMPTY_BRIDGE, clone_metadata_t, clone_metadata_t, EMPTY_RESUB, EMPTY_RECIRC>(ip, PacketReplicationEngine(), ep, BufferingQueueingEngine()) main;
