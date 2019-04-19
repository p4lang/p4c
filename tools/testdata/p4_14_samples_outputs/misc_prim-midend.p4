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
    bit<32> tmp;
    bit<32> tmp_0;
    int<32> tmp_1;
    int<32> tmp_2;
    int<32> tmp_3;
    @name(".NoAction") action NoAction_0() {
    }
    @name(".action_0") action action_0() {
        hdr.pkt.field_a_32 = ~((bit<32>)hdr.pkt.field_b_32 | hdr.pkt.field_c_32);
    }
    @name(".action_1") action action_1(bit<32> param0) {
        hdr.pkt.field_a_32 = ~(param0 & hdr.pkt.field_c_32);
    }
    @name(".action_2") action action_2(bit<32> param0) {
        hdr.pkt.field_a_32 = ~((bit<32>)hdr.pkt.field_b_32 ^ param0);
    }
    @name(".action_3") action action_3() {
        hdr.pkt.field_a_32 = ~hdr.pkt.field_d_32;
    }
    @name(".action_4") action action_4(bit<32> param0) {
        tmp = (hdr.pkt.field_d_32 <= param0 ? hdr.pkt.field_d_32 : tmp);
        tmp = (!(hdr.pkt.field_d_32 <= param0) ? param0 : tmp);
        hdr.pkt.field_a_32 = tmp;
    }
    @name(".action_5") action action_5(bit<32> param0) {
        tmp_0 = (param0 >= hdr.pkt.field_d_32 ? param0 : tmp_0);
        tmp_0 = (!(param0 >= hdr.pkt.field_d_32) ? hdr.pkt.field_d_32 : tmp_0);
        hdr.pkt.field_a_32 = tmp_0;
    }
    @name(".action_6") action action_6() {
        tmp_1 = ((int<32>)hdr.pkt.field_d_32 <= 32s7 ? (int<32>)hdr.pkt.field_d_32 : tmp_1);
        tmp_1 = (!((int<32>)hdr.pkt.field_d_32 <= 32s7) ? 32s7 : tmp_1);
        hdr.pkt.field_b_32 = tmp_1;
    }
    @name(".action_7") action action_7(int<32> param0) {
        tmp_2 = (param0 >= (int<32>)hdr.pkt.field_d_32 ? param0 : tmp_2);
        tmp_2 = (!(param0 >= (int<32>)hdr.pkt.field_d_32) ? (int<32>)hdr.pkt.field_d_32 : tmp_2);
        hdr.pkt.field_b_32 = tmp_2;
    }
    @name(".action_8") action action_8(int<32> param0) {
        tmp_3 = (hdr.pkt.field_x_32 >= param0 ? hdr.pkt.field_x_32 : tmp_3);
        tmp_3 = (!(hdr.pkt.field_x_32 >= param0) ? param0 : tmp_3);
        hdr.pkt.field_x_32 = tmp_3;
    }
    @name(".action_9") action action_9() {
        hdr.pkt.field_x_32 = hdr.pkt.field_x_32 >> 7;
    }
    @name(".action_10") action action_10(bit<32> param0) {
        hdr.pkt.field_a_32 = ~param0 & hdr.pkt.field_a_32;
    }
    @name(".action_11") action action_11(bit<32> param0) {
        hdr.pkt.field_a_32 = param0 & ~hdr.pkt.field_a_32;
    }
    @name(".action_12") action action_12(bit<32> param0) {
        hdr.pkt.field_a_32 = ~param0 | hdr.pkt.field_a_32;
    }
    @name(".action_13") action action_13(bit<32> param0) {
        hdr.pkt.field_a_32 = param0 | ~hdr.pkt.field_a_32;
    }
    @name(".do_nothing") action do_nothing() {
    }
    @name(".table_0") table table_1 {
        actions = {
            action_0();
            action_1();
            action_2();
            action_3();
            action_4();
            action_5();
            action_6();
            action_7();
            action_8();
            action_9();
            action_10();
            action_11();
            action_12();
            action_13();
            do_nothing();
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
        table_1.apply();
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

