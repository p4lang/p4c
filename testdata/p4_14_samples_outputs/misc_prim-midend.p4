#include <core.p4>
#include <v1model.p4>

header pkt_t {
    bit<32> field_a_32;
    int<32> field_b_32;
    bit<32> field_c_32;
    bit<32> field_d_32;
    bit<16> field_e_16;
    bit<16> field_f_16;
    bit<16> field_g_16;
    bit<16> field_h_16;
    bit<8>  field_i_8;
    bit<8>  field_j_8;
    bit<8>  field_k_8;
    bit<8>  field_l_8;
    int<32> field_x_32;
}

struct metadata {
}

struct headers {
    @name(".pkt") 
    pkt_t pkt;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".parse_ethernet") state parse_ethernet {
        packet.extract<pkt_t>(hdr.pkt);
        transition accept;
    }
    @name(".start") state start {
        transition parse_ethernet;
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    bit<32> tmp_4;
    bit<32> tmp_5;
    int<32> tmp_6;
    int<32> tmp_7;
    int<32> tmp_8;
    @name(".NoAction") action NoAction_0() {
    }
    @name(".action_0") action action_14() {
        hdr.pkt.field_a_32 = ~((bit<32>)hdr.pkt.field_b_32 | hdr.pkt.field_c_32);
    }
    @name(".action_1") action action_15(bit<32> param0) {
        hdr.pkt.field_a_32 = ~(param0 & hdr.pkt.field_c_32);
    }
    @name(".action_2") action action_16(bit<32> param0) {
        hdr.pkt.field_a_32 = ~((bit<32>)hdr.pkt.field_b_32 ^ param0);
    }
    @name(".action_3") action action_17() {
        hdr.pkt.field_a_32 = ~hdr.pkt.field_d_32;
    }
    @name(".action_4") action action_18(bit<32> param0) {
        tmp_4 = (hdr.pkt.field_d_32 <= param0 ? hdr.pkt.field_d_32 : tmp_4);
        tmp_4 = (!(hdr.pkt.field_d_32 <= param0) ? param0 : tmp_4);
        hdr.pkt.field_a_32 = tmp_4;
    }
    @name(".action_5") action action_19(bit<32> param0) {
        tmp_5 = (param0 >= hdr.pkt.field_d_32 ? param0 : tmp_5);
        tmp_5 = (!(param0 >= hdr.pkt.field_d_32) ? hdr.pkt.field_d_32 : tmp_5);
        hdr.pkt.field_a_32 = tmp_5;
    }
    @name(".action_6") action action_20() {
        tmp_6 = ((int<32>)hdr.pkt.field_d_32 <= 32s7 ? (int<32>)hdr.pkt.field_d_32 : tmp_6);
        tmp_6 = (!((int<32>)hdr.pkt.field_d_32 <= 32s7) ? 32s7 : tmp_6);
        hdr.pkt.field_b_32 = tmp_6;
    }
    @name(".action_7") action action_21(int<32> param0) {
        tmp_7 = (param0 >= (int<32>)hdr.pkt.field_d_32 ? param0 : tmp_7);
        tmp_7 = (!(param0 >= (int<32>)hdr.pkt.field_d_32) ? (int<32>)hdr.pkt.field_d_32 : tmp_7);
        hdr.pkt.field_b_32 = tmp_7;
    }
    @name(".action_8") action action_22(int<32> param0) {
        tmp_8 = (hdr.pkt.field_x_32 >= param0 ? hdr.pkt.field_x_32 : tmp_8);
        tmp_8 = (!(hdr.pkt.field_x_32 >= param0) ? param0 : tmp_8);
        hdr.pkt.field_x_32 = tmp_8;
    }
    @name(".action_9") action action_23() {
        hdr.pkt.field_x_32 = hdr.pkt.field_x_32 >> 7;
    }
    @name(".action_10") action action_24(bit<32> param0) {
        hdr.pkt.field_a_32 = ~param0 & hdr.pkt.field_a_32;
    }
    @name(".action_11") action action_25(bit<32> param0) {
        hdr.pkt.field_a_32 = param0 & ~hdr.pkt.field_a_32;
    }
    @name(".action_12") action action_26(bit<32> param0) {
        hdr.pkt.field_a_32 = ~param0 | hdr.pkt.field_a_32;
    }
    @name(".action_13") action action_27(bit<32> param0) {
        hdr.pkt.field_a_32 = param0 | ~hdr.pkt.field_a_32;
    }
    @name(".do_nothing") action do_nothing_0() {
    }
    @name(".table_0") table table_0 {
        actions = {
            action_14();
            action_15();
            action_16();
            action_17();
            action_18();
            action_19();
            action_20();
            action_21();
            action_22();
            action_23();
            action_24();
            action_25();
            action_26();
            action_27();
            do_nothing_0();
            @defaultonly NoAction_0();
        }
        key = {
            hdr.pkt.field_a_32: ternary @name("pkt.field_a_32") ;
            hdr.pkt.field_b_32: ternary @name("pkt.field_b_32") ;
            hdr.pkt.field_c_32: ternary @name("pkt.field_c_32") ;
            hdr.pkt.field_d_32: ternary @name("pkt.field_d_32") ;
            hdr.pkt.field_g_16: ternary @name("pkt.field_g_16") ;
            hdr.pkt.field_h_16: ternary @name("pkt.field_h_16") ;
        }
        size = 512;
        default_action = NoAction_0();
    }
    apply {
        table_0.apply();
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        packet.emit<pkt_t>(hdr.pkt);
    }
}

control verifyChecksum(inout headers hdr, inout metadata meta) {
    apply {
    }
}

control computeChecksum(inout headers hdr, inout metadata meta) {
    apply {
    }
}

V1Switch<headers, metadata>(ParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;

