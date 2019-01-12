error {
    BadHeaderType
}
#include <core.p4>
#include <v1model.p4>

header h1_t {
    bit<8> hdr_type;
    bit<8> op1;
    bit<8> op2;
    bit<8> op3;
    bit<8> h2_valid_bits;
    bit<8> next_hdr_type;
}

header h2_t {
    bit<8> hdr_type;
    bit<8> f1;
    bit<8> f2;
    bit<8> next_hdr_type;
}

header h3_t {
    bit<8> hdr_type;
    bit<8> data;
}

struct headers {
    h1_t    h1;
    h2_t[5] h2;
    h3_t    h3;
}

struct metadata {
}

parser parserI(packet_in pkt, out headers hdr, inout metadata meta, inout standard_metadata_t stdmeta) {
    h2_t[5] hdr_0_h2;
    state start {
        pkt.extract<h1_t>(hdr.h1);
        verify(hdr.h1.hdr_type == 8w1, error.BadHeaderType);
        transition select(hdr.h1.next_hdr_type) {
            8w2: parse_first_h2;
            8w3: parse_h3;
            default: accept;
        }
    }
    state parse_first_h2 {
        hdr_0_h2 = hdr.h2;
        pkt.extract<h2_t>(hdr_0_h2.next);
        verify(hdr_0_h2.last.hdr_type == 8w2, error.BadHeaderType);
        hdr.h2 = hdr_0_h2;
        transition select(hdr_0_h2.last.next_hdr_type) {
            8w2: parse_other_h2;
            8w3: parse_h3;
            default: accept;
        }
    }
    state parse_other_h2 {
        pkt.extract<h2_t>(hdr.h2.next);
        verify(hdr.h2.last.hdr_type == 8w2, error.BadHeaderType);
        transition select(hdr.h2.last.next_hdr_type) {
            8w2: parse_other_h2;
            8w3: parse_h3;
            default: accept;
        }
    }
    state parse_h3 {
        pkt.extract<h3_t>(hdr.h3);
        verify(hdr.h3.hdr_type == 8w3, error.BadHeaderType);
        transition accept;
    }
}

control cIngress(inout headers hdr, inout metadata meta, inout standard_metadata_t stdmeta) {
    @hidden action act() {
        hdr.h1.h2_valid_bits[0:0] = 1w1;
    }
    @hidden action act_0() {
        hdr.h1.h2_valid_bits = 8w0;
    }
    @hidden action act_1() {
        hdr.h1.h2_valid_bits[1:1] = 1w1;
    }
    @hidden action act_2() {
        hdr.h1.h2_valid_bits[2:2] = 1w1;
    }
    @hidden action act_3() {
        hdr.h1.h2_valid_bits[3:3] = 1w1;
    }
    @hidden action act_4() {
        hdr.h1.h2_valid_bits[4:4] = 1w1;
    }
    @hidden table tbl_act {
        actions = {
            act_0();
        }
        const default_action = act_0();
    }
    @hidden table tbl_act_0 {
        actions = {
            act();
        }
        const default_action = act();
    }
    @hidden table tbl_act_1 {
        actions = {
            act_1();
        }
        const default_action = act_1();
    }
    @hidden table tbl_act_2 {
        actions = {
            act_2();
        }
        const default_action = act_2();
    }
    @hidden table tbl_act_3 {
        actions = {
            act_3();
        }
        const default_action = act_3();
    }
    @hidden table tbl_act_4 {
        actions = {
            act_4();
        }
        const default_action = act_4();
    }
    apply {
        tbl_act.apply();
        if (hdr.h2[0].isValid()) 
            tbl_act_0.apply();
        if (hdr.h2[1].isValid()) 
            tbl_act_1.apply();
        if (hdr.h2[2].isValid()) 
            tbl_act_2.apply();
        if (hdr.h2[3].isValid()) 
            tbl_act_3.apply();
        if (hdr.h2[4].isValid()) 
            tbl_act_4.apply();
    }
}

control cEgress(inout headers hdr, inout metadata meta, inout standard_metadata_t stdmeta) {
    apply {
    }
}

control vc(inout headers hdr, inout metadata meta) {
    apply {
    }
}

control uc(inout headers hdr, inout metadata meta) {
    apply {
    }
}

control DeparserI(packet_out packet, in headers hdr) {
    apply {
        packet.emit<h1_t>(hdr.h1);
        packet.emit<h2_t>(hdr.h2[0]);
        packet.emit<h2_t>(hdr.h2[1]);
        packet.emit<h2_t>(hdr.h2[2]);
        packet.emit<h2_t>(hdr.h2[3]);
        packet.emit<h2_t>(hdr.h2[4]);
        packet.emit<h3_t>(hdr.h3);
    }
}

V1Switch<headers, metadata>(parserI(), vc(), cIngress(), cEgress(), uc(), DeparserI()) main;

