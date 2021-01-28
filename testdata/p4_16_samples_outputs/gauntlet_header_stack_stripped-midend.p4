error {
    BadHeaderType
}
#include <core.p4>
#define V1MODEL_VERSION 20180101
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
    state start {
        pkt.extract<h1_t>(hdr.h1);
        verify(hdr.h1.hdr_type == 8w1, error.BadHeaderType);
        transition select(hdr.h1.next_hdr_type) {
            8w2: parse_h2;
            8w3: parse_h3;
            default: accept;
        }
    }
    state parse_h2 {
        pkt.extract<h2_t>(hdr.h2.next);
        verify(hdr.h2.last.hdr_type == 8w2, error.BadHeaderType);
        transition select(hdr.h2.last.next_hdr_type) {
            8w2: parse_h2;
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
    h2_t[5] tmp_h2;
    @hidden action gauntlet_header_stack_stripped94() {
        tmp_h2.push_front(1);
    }
    @hidden action gauntlet_header_stack_stripped96() {
        tmp_h2.push_front(2);
    }
    @hidden action gauntlet_header_stack_stripped98() {
        tmp_h2.push_front(3);
    }
    @hidden action gauntlet_header_stack_stripped100() {
        tmp_h2.push_front(4);
    }
    @hidden action gauntlet_header_stack_stripped102() {
        tmp_h2.push_front(5);
    }
    @hidden action gauntlet_header_stack_stripped104() {
        tmp_h2.push_front(6);
    }
    @hidden action gauntlet_header_stack_stripped109() {
        tmp_h2.pop_front(1);
    }
    @hidden action gauntlet_header_stack_stripped111() {
        tmp_h2.pop_front(2);
    }
    @hidden action gauntlet_header_stack_stripped113() {
        tmp_h2.pop_front(3);
    }
    @hidden action gauntlet_header_stack_stripped115() {
        tmp_h2.pop_front(4);
    }
    @hidden action gauntlet_header_stack_stripped117() {
        tmp_h2.pop_front(5);
    }
    @hidden action gauntlet_header_stack_stripped119() {
        tmp_h2.pop_front(6);
    }
    @hidden action gauntlet_header_stack_stripped124() {
        tmp_h2[0].setValid();
        tmp_h2[0].hdr_type = 8w2;
        tmp_h2[0].f1 = 8w0xa0;
        tmp_h2[0].f2 = 8w0xa;
        tmp_h2[0].next_hdr_type = 8w9;
    }
    @hidden action gauntlet_header_stack_stripped130() {
        tmp_h2[1].setValid();
        tmp_h2[1].hdr_type = 8w2;
        tmp_h2[1].f1 = 8w0xa1;
        tmp_h2[1].f2 = 8w0x1a;
        tmp_h2[1].next_hdr_type = 8w9;
    }
    @hidden action gauntlet_header_stack_stripped136() {
        tmp_h2[2].setValid();
        tmp_h2[2].hdr_type = 8w2;
        tmp_h2[2].f1 = 8w0xa2;
        tmp_h2[2].f2 = 8w0x2a;
        tmp_h2[2].next_hdr_type = 8w9;
    }
    @hidden action gauntlet_header_stack_stripped142() {
        tmp_h2[3].setValid();
        tmp_h2[3].hdr_type = 8w2;
        tmp_h2[3].f1 = 8w0xa3;
        tmp_h2[3].f2 = 8w0x3a;
        tmp_h2[3].next_hdr_type = 8w9;
    }
    @hidden action gauntlet_header_stack_stripped148() {
        tmp_h2[4].setValid();
        tmp_h2[4].hdr_type = 8w2;
        tmp_h2[4].f1 = 8w0xa4;
        tmp_h2[4].f2 = 8w0x4a;
        tmp_h2[4].next_hdr_type = 8w9;
    }
    @hidden action gauntlet_header_stack_stripped157() {
        tmp_h2[0].setInvalid();
    }
    @hidden action gauntlet_header_stack_stripped159() {
        tmp_h2[1].setInvalid();
    }
    @hidden action gauntlet_header_stack_stripped161() {
        tmp_h2[2].setInvalid();
    }
    @hidden action gauntlet_header_stack_stripped163() {
        tmp_h2[3].setInvalid();
    }
    @hidden action gauntlet_header_stack_stripped165() {
        tmp_h2[4].setInvalid();
    }
    @hidden action act() {
        tmp_h2 = hdr.h2;
    }
    @hidden action gauntlet_header_stack_stripped181() {
        hdr.h2 = tmp_h2;
        hdr.h1.h2_valid_bits = 8w0;
    }
    @hidden table tbl_act {
        actions = {
            act();
        }
        const default_action = act();
    }
    @hidden table tbl_gauntlet_header_stack_stripped94 {
        actions = {
            gauntlet_header_stack_stripped94();
        }
        const default_action = gauntlet_header_stack_stripped94();
    }
    @hidden table tbl_gauntlet_header_stack_stripped96 {
        actions = {
            gauntlet_header_stack_stripped96();
        }
        const default_action = gauntlet_header_stack_stripped96();
    }
    @hidden table tbl_gauntlet_header_stack_stripped98 {
        actions = {
            gauntlet_header_stack_stripped98();
        }
        const default_action = gauntlet_header_stack_stripped98();
    }
    @hidden table tbl_gauntlet_header_stack_stripped100 {
        actions = {
            gauntlet_header_stack_stripped100();
        }
        const default_action = gauntlet_header_stack_stripped100();
    }
    @hidden table tbl_gauntlet_header_stack_stripped102 {
        actions = {
            gauntlet_header_stack_stripped102();
        }
        const default_action = gauntlet_header_stack_stripped102();
    }
    @hidden table tbl_gauntlet_header_stack_stripped104 {
        actions = {
            gauntlet_header_stack_stripped104();
        }
        const default_action = gauntlet_header_stack_stripped104();
    }
    @hidden table tbl_gauntlet_header_stack_stripped109 {
        actions = {
            gauntlet_header_stack_stripped109();
        }
        const default_action = gauntlet_header_stack_stripped109();
    }
    @hidden table tbl_gauntlet_header_stack_stripped111 {
        actions = {
            gauntlet_header_stack_stripped111();
        }
        const default_action = gauntlet_header_stack_stripped111();
    }
    @hidden table tbl_gauntlet_header_stack_stripped113 {
        actions = {
            gauntlet_header_stack_stripped113();
        }
        const default_action = gauntlet_header_stack_stripped113();
    }
    @hidden table tbl_gauntlet_header_stack_stripped115 {
        actions = {
            gauntlet_header_stack_stripped115();
        }
        const default_action = gauntlet_header_stack_stripped115();
    }
    @hidden table tbl_gauntlet_header_stack_stripped117 {
        actions = {
            gauntlet_header_stack_stripped117();
        }
        const default_action = gauntlet_header_stack_stripped117();
    }
    @hidden table tbl_gauntlet_header_stack_stripped119 {
        actions = {
            gauntlet_header_stack_stripped119();
        }
        const default_action = gauntlet_header_stack_stripped119();
    }
    @hidden table tbl_gauntlet_header_stack_stripped124 {
        actions = {
            gauntlet_header_stack_stripped124();
        }
        const default_action = gauntlet_header_stack_stripped124();
    }
    @hidden table tbl_gauntlet_header_stack_stripped130 {
        actions = {
            gauntlet_header_stack_stripped130();
        }
        const default_action = gauntlet_header_stack_stripped130();
    }
    @hidden table tbl_gauntlet_header_stack_stripped136 {
        actions = {
            gauntlet_header_stack_stripped136();
        }
        const default_action = gauntlet_header_stack_stripped136();
    }
    @hidden table tbl_gauntlet_header_stack_stripped142 {
        actions = {
            gauntlet_header_stack_stripped142();
        }
        const default_action = gauntlet_header_stack_stripped142();
    }
    @hidden table tbl_gauntlet_header_stack_stripped148 {
        actions = {
            gauntlet_header_stack_stripped148();
        }
        const default_action = gauntlet_header_stack_stripped148();
    }
    @hidden table tbl_gauntlet_header_stack_stripped157 {
        actions = {
            gauntlet_header_stack_stripped157();
        }
        const default_action = gauntlet_header_stack_stripped157();
    }
    @hidden table tbl_gauntlet_header_stack_stripped159 {
        actions = {
            gauntlet_header_stack_stripped159();
        }
        const default_action = gauntlet_header_stack_stripped159();
    }
    @hidden table tbl_gauntlet_header_stack_stripped161 {
        actions = {
            gauntlet_header_stack_stripped161();
        }
        const default_action = gauntlet_header_stack_stripped161();
    }
    @hidden table tbl_gauntlet_header_stack_stripped163 {
        actions = {
            gauntlet_header_stack_stripped163();
        }
        const default_action = gauntlet_header_stack_stripped163();
    }
    @hidden table tbl_gauntlet_header_stack_stripped165 {
        actions = {
            gauntlet_header_stack_stripped165();
        }
        const default_action = gauntlet_header_stack_stripped165();
    }
    @hidden table tbl_gauntlet_header_stack_stripped181 {
        actions = {
            gauntlet_header_stack_stripped181();
        }
        const default_action = gauntlet_header_stack_stripped181();
    }
    apply {
        tbl_act.apply();
        if (hdr.h1.op1 == 8w0x0) {
            ;
        } else if (hdr.h1.op1[7:4] == 4w1) {
            if (hdr.h1.op1[3:0] == 4w1) {
                tbl_gauntlet_header_stack_stripped94.apply();
            } else if (hdr.h1.op1[3:0] == 4w2) {
                tbl_gauntlet_header_stack_stripped96.apply();
            } else if (hdr.h1.op1[3:0] == 4w3) {
                tbl_gauntlet_header_stack_stripped98.apply();
            } else if (hdr.h1.op1[3:0] == 4w4) {
                tbl_gauntlet_header_stack_stripped100.apply();
            } else if (hdr.h1.op1[3:0] == 4w5) {
                tbl_gauntlet_header_stack_stripped102.apply();
            } else if (hdr.h1.op1[3:0] == 4w6) {
                tbl_gauntlet_header_stack_stripped104.apply();
            }
        } else if (hdr.h1.op1[7:4] == 4w2) {
            if (hdr.h1.op1[3:0] == 4w1) {
                tbl_gauntlet_header_stack_stripped109.apply();
            } else if (hdr.h1.op1[3:0] == 4w2) {
                tbl_gauntlet_header_stack_stripped111.apply();
            } else if (hdr.h1.op1[3:0] == 4w3) {
                tbl_gauntlet_header_stack_stripped113.apply();
            } else if (hdr.h1.op1[3:0] == 4w4) {
                tbl_gauntlet_header_stack_stripped115.apply();
            } else if (hdr.h1.op1[3:0] == 4w5) {
                tbl_gauntlet_header_stack_stripped117.apply();
            } else if (hdr.h1.op1[3:0] == 4w6) {
                tbl_gauntlet_header_stack_stripped119.apply();
            }
        } else if (hdr.h1.op1[7:4] == 4w3) {
            if (hdr.h1.op1[3:0] == 4w0) {
                tbl_gauntlet_header_stack_stripped124.apply();
            } else if (hdr.h1.op1[3:0] == 4w1) {
                tbl_gauntlet_header_stack_stripped130.apply();
            } else if (hdr.h1.op1[3:0] == 4w2) {
                tbl_gauntlet_header_stack_stripped136.apply();
            } else if (hdr.h1.op1[3:0] == 4w3) {
                tbl_gauntlet_header_stack_stripped142.apply();
            } else if (hdr.h1.op1[3:0] == 4w4) {
                tbl_gauntlet_header_stack_stripped148.apply();
            }
        } else if (hdr.h1.op1[7:4] == 4w4) {
            if (hdr.h1.op1[3:0] == 4w0) {
                tbl_gauntlet_header_stack_stripped157.apply();
            } else if (hdr.h1.op1[3:0] == 4w1) {
                tbl_gauntlet_header_stack_stripped159.apply();
            } else if (hdr.h1.op1[3:0] == 4w2) {
                tbl_gauntlet_header_stack_stripped161.apply();
            } else if (hdr.h1.op1[3:0] == 4w3) {
                tbl_gauntlet_header_stack_stripped163.apply();
            } else if (hdr.h1.op1[3:0] == 4w4) {
                tbl_gauntlet_header_stack_stripped165.apply();
            }
        }
        tbl_gauntlet_header_stack_stripped181.apply();
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

