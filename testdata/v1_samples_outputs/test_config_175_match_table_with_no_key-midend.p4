#include "/home/mbudiu/barefoot/git/p4c/build/../p4include/core.p4"
#include "/home/mbudiu/barefoot/git/p4c/build/../p4include/v1model.p4"

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
    @name("pkt") 
    pkt_t pkt;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name("parse_ethernet") state parse_ethernet {
        packet.extract(hdr.pkt);
        transition accept;
    }
    @name("start") state start {
        transition parse_ethernet;
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    action NoAction_0() {
    }
    @name("action_0") action action() {
        hdr.pkt.field_a_32 = (bit<32>)~(hdr.pkt.field_b_32 | (int<32>)hdr.pkt.field_c_32);
    }
    @name("action_1") action action_2(int<32> param0) {
        hdr.pkt.field_b_32 = ~param0 | (int<32>)hdr.pkt.field_c_32;
    }
    @name("do_nothing") action do_nothing_0() {
    }
    @name("table_0") table table() {
        actions = {
            action;
            NoAction_0;
        }
        default_action = NoAction_0();
    }
    @name("table_1") table table_3() {
        actions = {
            action_2;
            NoAction_0;
        }
        default_action = NoAction_0();
    }
    @name("table_2") table table_4() {
        actions = {
            do_nothing_0;
            NoAction_0;
        }
        default_action = NoAction_0();
    }
    apply {
        table.apply();
        table_3.apply();
        if (hdr.pkt.field_i_8 == 8w0) 
            table_4.apply();
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        packet.emit(hdr.pkt);
    }
}

control verifyChecksum(in headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control computeChecksum(inout headers hdr, inout metadata meta) {
    apply {
    }
}

V1Switch(ParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;
