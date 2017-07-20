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

struct __metadataImpl {
    @name("standard_metadata") 
    standard_metadata_t standard_metadata;
}

struct __headersImpl {
    @name("pkt") 
    pkt_t pkt;
}

parser __ParserImpl(packet_in packet, out __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t __standard_metadata) {
    @name(".parse_ethernet") state parse_ethernet {
        packet.extract<pkt_t>(hdr.pkt);
        transition accept;
    }
    @name(".start") state start {
        transition parse_ethernet;
    }
}

control ingress(inout __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t __standard_metadata) {
    @name("NoAction") action NoAction_0() {
    }
    @name("NoAction") action NoAction_4() {
    }
    @name("NoAction") action NoAction_5() {
    }
    @name(".action_0") action action_2() {
        hdr.pkt.field_a_32 = ~((bit<32>)hdr.pkt.field_b_32 | hdr.pkt.field_c_32);
    }
    @name(".action_1") action action_3(int<32> param0) {
        hdr.pkt.field_b_32 = ~param0 | (int<32>)hdr.pkt.field_c_32;
    }
    @name(".do_nothing") action do_nothing_0() {
    }
    @name(".table_0") table table_0 {
        actions = {
            action_2();
            @defaultonly NoAction_0();
        }
        default_action = NoAction_0();
    }
    @name(".table_1") table table_1 {
        actions = {
            action_3();
            @defaultonly NoAction_4();
        }
        default_action = NoAction_4();
    }
    @name(".table_2") table table_2 {
        actions = {
            do_nothing_0();
            @defaultonly NoAction_5();
        }
        default_action = NoAction_5();
    }
    apply {
        table_0.apply();
        table_1.apply();
        if (hdr.pkt.field_i_8 == 8w0) 
            table_2.apply();
    }
}

control __egressImpl(inout __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t __standard_metadata) {
    apply {
    }
}

control __DeparserImpl(packet_out packet, in __headersImpl hdr) {
    apply {
        packet.emit<pkt_t>(hdr.pkt);
    }
}

control __verifyChecksumImpl(in __headersImpl hdr, inout __metadataImpl meta) {
    apply {
    }
}

control __computeChecksumImpl(inout __headersImpl hdr, inout __metadataImpl meta) {
    apply {
    }
}

V1Switch<__headersImpl, __metadataImpl>(__ParserImpl(), __verifyChecksumImpl(), ingress(), __egressImpl(), __computeChecksumImpl(), __DeparserImpl()) main;
