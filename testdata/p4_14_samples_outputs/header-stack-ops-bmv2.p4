#include <core.p4>
#define V1MODEL_VERSION 20200408
#include <v1model.p4>

struct metadata_t {
    bit<8> op;
}

header h1_t {
    bit<8> hdr_type;
    bit<8> op1;
    bit<8> op2;
    bit<8> op3;
    bit<8> h2_valid_bits;
    bit<8> next_hdr_type;
}

header h3_t {
    bit<8> hdr_type;
    bit<8> data;
}

header h2_t {
    bit<8> hdr_type;
    bit<8> f1;
    bit<8> f2;
    bit<8> next_hdr_type;
}

struct metadata {
    @name(".m") 
    metadata_t m;
}

struct headers {
    @name(".h1") 
    h1_t    h1;
    @name(".h3") 
    h3_t    h3;
    @name(".h2") 
    h2_t[5] h2;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".parse_h2") state parse_h2 {
        packet.extract(hdr.h2.next);
        transition select(hdr.h2.last.next_hdr_type) {
            8w2: parse_h2;
            8w3: parse_h3;
            default: accept;
        }
    }
    @name(".parse_h3") state parse_h3 {
        packet.extract(hdr.h3);
        transition accept;
    }
    @name(".start") state start {
        packet.extract(hdr.h1);
        transition select(hdr.h1.next_hdr_type) {
            8w2: parse_h2;
            8w3: parse_h3;
            default: accept;
        }
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control op1_do(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".op1_a_add_header_0") action op1_a_add_header_0() {
        hdr.h2[0].setValid();
    }
    @name(".op1_a_add_header_1") action op1_a_add_header_1() {
        hdr.h2[1].setValid();
    }
    @name(".op1_a_add_header_2") action op1_a_add_header_2() {
        hdr.h2[2].setValid();
    }
    @name(".op1_a_add_header_3") action op1_a_add_header_3() {
        hdr.h2[3].setValid();
    }
    @name(".op1_a_add_header_4") action op1_a_add_header_4() {
        hdr.h2[4].setValid();
    }
    @name(".op1_a_assign_header_0") action op1_a_assign_header_0() {
        hdr.h2[0].hdr_type = 8w2;
        hdr.h2[0].f1 = 8w0xa0;
        hdr.h2[0].f2 = 8w0xa;
        hdr.h2[0].next_hdr_type = 8w9;
    }
    @name(".op1_a_assign_header_1") action op1_a_assign_header_1() {
        hdr.h2[1].hdr_type = 8w2;
        hdr.h2[1].f1 = 8w0xa1;
        hdr.h2[1].f2 = 8w0x1a;
        hdr.h2[1].next_hdr_type = 8w9;
    }
    @name(".op1_a_assign_header_2") action op1_a_assign_header_2() {
        hdr.h2[2].hdr_type = 8w2;
        hdr.h2[2].f1 = 8w0xa2;
        hdr.h2[2].f2 = 8w0x2a;
        hdr.h2[2].next_hdr_type = 8w9;
    }
    @name(".op1_a_assign_header_3") action op1_a_assign_header_3() {
        hdr.h2[3].hdr_type = 8w2;
        hdr.h2[3].f1 = 8w0xa3;
        hdr.h2[3].f2 = 8w0x3a;
        hdr.h2[3].next_hdr_type = 8w9;
    }
    @name(".op1_a_assign_header_4") action op1_a_assign_header_4() {
        hdr.h2[4].hdr_type = 8w2;
        hdr.h2[4].f1 = 8w0xa4;
        hdr.h2[4].f2 = 8w0x4a;
        hdr.h2[4].next_hdr_type = 8w9;
    }
    @name(".op1_a_pop_1") action op1_a_pop_1() {
        hdr.h2.pop_front(1);
    }
    @name(".op1_a_pop_2") action op1_a_pop_2() {
        hdr.h2.pop_front(2);
    }
    @name(".op1_a_pop_3") action op1_a_pop_3() {
        hdr.h2.pop_front(3);
    }
    @name(".op1_a_pop_4") action op1_a_pop_4() {
        hdr.h2.pop_front(4);
    }
    @name(".op1_a_pop_5") action op1_a_pop_5() {
        hdr.h2.pop_front(5);
    }
    @name(".op1_a_push_1") action op1_a_push_1() {
        {
            hdr.h2.push_front(1);
            hdr.h2[0].setValid();
        }
    }
    @name(".op1_a_push_2") action op1_a_push_2() {
        {
            hdr.h2.push_front(2);
            hdr.h2[0].setValid();
            hdr.h2[1].setValid();
        }
    }
    @name(".op1_a_push_3") action op1_a_push_3() {
        {
            hdr.h2.push_front(3);
            hdr.h2[0].setValid();
            hdr.h2[1].setValid();
            hdr.h2[2].setValid();
        }
    }
    @name(".op1_a_push_4") action op1_a_push_4() {
        {
            hdr.h2.push_front(4);
            hdr.h2[0].setValid();
            hdr.h2[1].setValid();
            hdr.h2[2].setValid();
            hdr.h2[3].setValid();
        }
    }
    @name(".op1_a_push_5") action op1_a_push_5() {
        {
            hdr.h2.push_front(5);
            hdr.h2[0].setValid();
            hdr.h2[1].setValid();
            hdr.h2[2].setValid();
            hdr.h2[3].setValid();
            hdr.h2[4].setValid();
        }
    }
    @name(".op1_a_remove_header_0") action op1_a_remove_header_0() {
        hdr.h2[0].setInvalid();
    }
    @name(".op1_a_remove_header_1") action op1_a_remove_header_1() {
        hdr.h2[1].setInvalid();
    }
    @name(".op1_a_remove_header_2") action op1_a_remove_header_2() {
        hdr.h2[2].setInvalid();
    }
    @name(".op1_a_remove_header_3") action op1_a_remove_header_3() {
        hdr.h2[3].setInvalid();
    }
    @name(".op1_a_remove_header_4") action op1_a_remove_header_4() {
        hdr.h2[4].setInvalid();
    }
    @name(".op1_t_add_header_0") table op1_t_add_header_0 {
        actions = {
            op1_a_add_header_0;
        }
        default_action = op1_a_add_header_0();
    }
    @name(".op1_t_add_header_1") table op1_t_add_header_1 {
        actions = {
            op1_a_add_header_1;
        }
        default_action = op1_a_add_header_1();
    }
    @name(".op1_t_add_header_2") table op1_t_add_header_2 {
        actions = {
            op1_a_add_header_2;
        }
        default_action = op1_a_add_header_2();
    }
    @name(".op1_t_add_header_3") table op1_t_add_header_3 {
        actions = {
            op1_a_add_header_3;
        }
        default_action = op1_a_add_header_3();
    }
    @name(".op1_t_add_header_4") table op1_t_add_header_4 {
        actions = {
            op1_a_add_header_4;
        }
        default_action = op1_a_add_header_4();
    }
    @name(".op1_t_assign_header_0") table op1_t_assign_header_0 {
        actions = {
            op1_a_assign_header_0;
        }
        default_action = op1_a_assign_header_0();
    }
    @name(".op1_t_assign_header_1") table op1_t_assign_header_1 {
        actions = {
            op1_a_assign_header_1;
        }
        default_action = op1_a_assign_header_1();
    }
    @name(".op1_t_assign_header_2") table op1_t_assign_header_2 {
        actions = {
            op1_a_assign_header_2;
        }
        default_action = op1_a_assign_header_2();
    }
    @name(".op1_t_assign_header_3") table op1_t_assign_header_3 {
        actions = {
            op1_a_assign_header_3;
        }
        default_action = op1_a_assign_header_3();
    }
    @name(".op1_t_assign_header_4") table op1_t_assign_header_4 {
        actions = {
            op1_a_assign_header_4;
        }
        default_action = op1_a_assign_header_4();
    }
    @name(".op1_t_pop_1") table op1_t_pop_1 {
        actions = {
            op1_a_pop_1;
        }
        default_action = op1_a_pop_1();
    }
    @name(".op1_t_pop_2") table op1_t_pop_2 {
        actions = {
            op1_a_pop_2;
        }
        default_action = op1_a_pop_2();
    }
    @name(".op1_t_pop_3") table op1_t_pop_3 {
        actions = {
            op1_a_pop_3;
        }
        default_action = op1_a_pop_3();
    }
    @name(".op1_t_pop_4") table op1_t_pop_4 {
        actions = {
            op1_a_pop_4;
        }
        default_action = op1_a_pop_4();
    }
    @name(".op1_t_pop_5") table op1_t_pop_5 {
        actions = {
            op1_a_pop_5;
        }
        default_action = op1_a_pop_5();
    }
    @name(".op1_t_push_1") table op1_t_push_1 {
        actions = {
            op1_a_push_1;
        }
        default_action = op1_a_push_1();
    }
    @name(".op1_t_push_2") table op1_t_push_2 {
        actions = {
            op1_a_push_2;
        }
        default_action = op1_a_push_2();
    }
    @name(".op1_t_push_3") table op1_t_push_3 {
        actions = {
            op1_a_push_3;
        }
        default_action = op1_a_push_3();
    }
    @name(".op1_t_push_4") table op1_t_push_4 {
        actions = {
            op1_a_push_4;
        }
        default_action = op1_a_push_4();
    }
    @name(".op1_t_push_5") table op1_t_push_5 {
        actions = {
            op1_a_push_5;
        }
        default_action = op1_a_push_5();
    }
    @name(".op1_t_remove_header_0") table op1_t_remove_header_0 {
        actions = {
            op1_a_remove_header_0;
        }
        default_action = op1_a_remove_header_0();
    }
    @name(".op1_t_remove_header_1") table op1_t_remove_header_1 {
        actions = {
            op1_a_remove_header_1;
        }
        default_action = op1_a_remove_header_1();
    }
    @name(".op1_t_remove_header_2") table op1_t_remove_header_2 {
        actions = {
            op1_a_remove_header_2;
        }
        default_action = op1_a_remove_header_2();
    }
    @name(".op1_t_remove_header_3") table op1_t_remove_header_3 {
        actions = {
            op1_a_remove_header_3;
        }
        default_action = op1_a_remove_header_3();
    }
    @name(".op1_t_remove_header_4") table op1_t_remove_header_4 {
        actions = {
            op1_a_remove_header_4;
        }
        default_action = op1_a_remove_header_4();
    }
    apply {
        if (meta.m.op == 8w0x0) {
        } else {
            if (meta.m.op >> 4 == 8w1) {
                if (meta.m.op & 8w0xf == 8w1) {
                    op1_t_push_1.apply();
                } else {
                    if (meta.m.op & 8w0xf == 8w2) {
                        op1_t_push_2.apply();
                    } else {
                        if (meta.m.op & 8w0xf == 8w3) {
                            op1_t_push_3.apply();
                        } else {
                            if (meta.m.op & 8w0xf == 8w4) {
                                op1_t_push_4.apply();
                            } else {
                                if (meta.m.op & 8w0xf == 8w5) {
                                    op1_t_push_5.apply();
                                }
                            }
                        }
                    }
                }
            } else {
                if (meta.m.op >> 4 == 8w2) {
                    if (meta.m.op & 8w0xf == 8w1) {
                        op1_t_pop_1.apply();
                    } else {
                        if (meta.m.op & 8w0xf == 8w2) {
                            op1_t_pop_2.apply();
                        } else {
                            if (meta.m.op & 8w0xf == 8w3) {
                                op1_t_pop_3.apply();
                            } else {
                                if (meta.m.op & 8w0xf == 8w4) {
                                    op1_t_pop_4.apply();
                                } else {
                                    if (meta.m.op & 8w0xf == 8w5) {
                                        op1_t_pop_5.apply();
                                    }
                                }
                            }
                        }
                    }
                } else {
                    if (meta.m.op >> 4 == 8w3) {
                        if (meta.m.op & 8w0xf == 8w0) {
                            op1_t_assign_header_0.apply();
                        } else {
                            if (meta.m.op & 8w0xf == 8w1) {
                                op1_t_assign_header_1.apply();
                            } else {
                                if (meta.m.op & 8w0xf == 8w2) {
                                    op1_t_assign_header_2.apply();
                                } else {
                                    if (meta.m.op & 8w0xf == 8w3) {
                                        op1_t_assign_header_3.apply();
                                    } else {
                                        if (meta.m.op & 8w0xf == 8w4) {
                                            op1_t_assign_header_4.apply();
                                        }
                                    }
                                }
                            }
                        }
                    } else {
                        if (meta.m.op >> 4 == 8w4) {
                            if (meta.m.op & 8w0xf == 8w0) {
                                op1_t_remove_header_0.apply();
                            } else {
                                if (meta.m.op & 8w0xf == 8w1) {
                                    op1_t_remove_header_1.apply();
                                } else {
                                    if (meta.m.op & 8w0xf == 8w2) {
                                        op1_t_remove_header_2.apply();
                                    } else {
                                        if (meta.m.op & 8w0xf == 8w3) {
                                            op1_t_remove_header_3.apply();
                                        } else {
                                            if (meta.m.op & 8w0xf == 8w4) {
                                                op1_t_remove_header_4.apply();
                                            }
                                        }
                                    }
                                }
                            }
                        } else {
                            if (meta.m.op >> 4 == 8w5) {
                                if (meta.m.op & 8w0xf == 8w0) {
                                    op1_t_add_header_0.apply();
                                } else {
                                    if (meta.m.op & 8w0xf == 8w1) {
                                        op1_t_add_header_1.apply();
                                    } else {
                                        if (meta.m.op & 8w0xf == 8w2) {
                                            op1_t_add_header_2.apply();
                                        } else {
                                            if (meta.m.op & 8w0xf == 8w3) {
                                                op1_t_add_header_3.apply();
                                            } else {
                                                if (meta.m.op & 8w0xf == 8w4) {
                                                    op1_t_add_header_4.apply();
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

control op2_do(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".op2_a_add_header_0") action op2_a_add_header_0() {
        hdr.h2[0].setValid();
    }
    @name(".op2_a_add_header_1") action op2_a_add_header_1() {
        hdr.h2[1].setValid();
    }
    @name(".op2_a_add_header_2") action op2_a_add_header_2() {
        hdr.h2[2].setValid();
    }
    @name(".op2_a_add_header_3") action op2_a_add_header_3() {
        hdr.h2[3].setValid();
    }
    @name(".op2_a_add_header_4") action op2_a_add_header_4() {
        hdr.h2[4].setValid();
    }
    @name(".op2_a_assign_header_0") action op2_a_assign_header_0() {
        hdr.h2[0].hdr_type = 8w2;
        hdr.h2[0].f1 = 8w0xa0;
        hdr.h2[0].f2 = 8w0xa;
        hdr.h2[0].next_hdr_type = 8w9;
    }
    @name(".op2_a_assign_header_1") action op2_a_assign_header_1() {
        hdr.h2[1].hdr_type = 8w2;
        hdr.h2[1].f1 = 8w0xa1;
        hdr.h2[1].f2 = 8w0x1a;
        hdr.h2[1].next_hdr_type = 8w9;
    }
    @name(".op2_a_assign_header_2") action op2_a_assign_header_2() {
        hdr.h2[2].hdr_type = 8w2;
        hdr.h2[2].f1 = 8w0xa2;
        hdr.h2[2].f2 = 8w0x2a;
        hdr.h2[2].next_hdr_type = 8w9;
    }
    @name(".op2_a_assign_header_3") action op2_a_assign_header_3() {
        hdr.h2[3].hdr_type = 8w2;
        hdr.h2[3].f1 = 8w0xa3;
        hdr.h2[3].f2 = 8w0x3a;
        hdr.h2[3].next_hdr_type = 8w9;
    }
    @name(".op2_a_assign_header_4") action op2_a_assign_header_4() {
        hdr.h2[4].hdr_type = 8w2;
        hdr.h2[4].f1 = 8w0xa4;
        hdr.h2[4].f2 = 8w0x4a;
        hdr.h2[4].next_hdr_type = 8w9;
    }
    @name(".op2_a_pop_1") action op2_a_pop_1() {
        hdr.h2.pop_front(1);
    }
    @name(".op2_a_pop_2") action op2_a_pop_2() {
        hdr.h2.pop_front(2);
    }
    @name(".op2_a_pop_3") action op2_a_pop_3() {
        hdr.h2.pop_front(3);
    }
    @name(".op2_a_pop_4") action op2_a_pop_4() {
        hdr.h2.pop_front(4);
    }
    @name(".op2_a_pop_5") action op2_a_pop_5() {
        hdr.h2.pop_front(5);
    }
    @name(".op2_a_push_1") action op2_a_push_1() {
        {
            hdr.h2.push_front(1);
            hdr.h2[0].setValid();
        }
    }
    @name(".op2_a_push_2") action op2_a_push_2() {
        {
            hdr.h2.push_front(2);
            hdr.h2[0].setValid();
            hdr.h2[1].setValid();
        }
    }
    @name(".op2_a_push_3") action op2_a_push_3() {
        {
            hdr.h2.push_front(3);
            hdr.h2[0].setValid();
            hdr.h2[1].setValid();
            hdr.h2[2].setValid();
        }
    }
    @name(".op2_a_push_4") action op2_a_push_4() {
        {
            hdr.h2.push_front(4);
            hdr.h2[0].setValid();
            hdr.h2[1].setValid();
            hdr.h2[2].setValid();
            hdr.h2[3].setValid();
        }
    }
    @name(".op2_a_push_5") action op2_a_push_5() {
        {
            hdr.h2.push_front(5);
            hdr.h2[0].setValid();
            hdr.h2[1].setValid();
            hdr.h2[2].setValid();
            hdr.h2[3].setValid();
            hdr.h2[4].setValid();
        }
    }
    @name(".op2_a_remove_header_0") action op2_a_remove_header_0() {
        hdr.h2[0].setInvalid();
    }
    @name(".op2_a_remove_header_1") action op2_a_remove_header_1() {
        hdr.h2[1].setInvalid();
    }
    @name(".op2_a_remove_header_2") action op2_a_remove_header_2() {
        hdr.h2[2].setInvalid();
    }
    @name(".op2_a_remove_header_3") action op2_a_remove_header_3() {
        hdr.h2[3].setInvalid();
    }
    @name(".op2_a_remove_header_4") action op2_a_remove_header_4() {
        hdr.h2[4].setInvalid();
    }
    @name(".op2_t_add_header_0") table op2_t_add_header_0 {
        actions = {
            op2_a_add_header_0;
        }
        default_action = op2_a_add_header_0();
    }
    @name(".op2_t_add_header_1") table op2_t_add_header_1 {
        actions = {
            op2_a_add_header_1;
        }
        default_action = op2_a_add_header_1();
    }
    @name(".op2_t_add_header_2") table op2_t_add_header_2 {
        actions = {
            op2_a_add_header_2;
        }
        default_action = op2_a_add_header_2();
    }
    @name(".op2_t_add_header_3") table op2_t_add_header_3 {
        actions = {
            op2_a_add_header_3;
        }
        default_action = op2_a_add_header_3();
    }
    @name(".op2_t_add_header_4") table op2_t_add_header_4 {
        actions = {
            op2_a_add_header_4;
        }
        default_action = op2_a_add_header_4();
    }
    @name(".op2_t_assign_header_0") table op2_t_assign_header_0 {
        actions = {
            op2_a_assign_header_0;
        }
        default_action = op2_a_assign_header_0();
    }
    @name(".op2_t_assign_header_1") table op2_t_assign_header_1 {
        actions = {
            op2_a_assign_header_1;
        }
        default_action = op2_a_assign_header_1();
    }
    @name(".op2_t_assign_header_2") table op2_t_assign_header_2 {
        actions = {
            op2_a_assign_header_2;
        }
        default_action = op2_a_assign_header_2();
    }
    @name(".op2_t_assign_header_3") table op2_t_assign_header_3 {
        actions = {
            op2_a_assign_header_3;
        }
        default_action = op2_a_assign_header_3();
    }
    @name(".op2_t_assign_header_4") table op2_t_assign_header_4 {
        actions = {
            op2_a_assign_header_4;
        }
        default_action = op2_a_assign_header_4();
    }
    @name(".op2_t_pop_1") table op2_t_pop_1 {
        actions = {
            op2_a_pop_1;
        }
        default_action = op2_a_pop_1();
    }
    @name(".op2_t_pop_2") table op2_t_pop_2 {
        actions = {
            op2_a_pop_2;
        }
        default_action = op2_a_pop_2();
    }
    @name(".op2_t_pop_3") table op2_t_pop_3 {
        actions = {
            op2_a_pop_3;
        }
        default_action = op2_a_pop_3();
    }
    @name(".op2_t_pop_4") table op2_t_pop_4 {
        actions = {
            op2_a_pop_4;
        }
        default_action = op2_a_pop_4();
    }
    @name(".op2_t_pop_5") table op2_t_pop_5 {
        actions = {
            op2_a_pop_5;
        }
        default_action = op2_a_pop_5();
    }
    @name(".op2_t_push_1") table op2_t_push_1 {
        actions = {
            op2_a_push_1;
        }
        default_action = op2_a_push_1();
    }
    @name(".op2_t_push_2") table op2_t_push_2 {
        actions = {
            op2_a_push_2;
        }
        default_action = op2_a_push_2();
    }
    @name(".op2_t_push_3") table op2_t_push_3 {
        actions = {
            op2_a_push_3;
        }
        default_action = op2_a_push_3();
    }
    @name(".op2_t_push_4") table op2_t_push_4 {
        actions = {
            op2_a_push_4;
        }
        default_action = op2_a_push_4();
    }
    @name(".op2_t_push_5") table op2_t_push_5 {
        actions = {
            op2_a_push_5;
        }
        default_action = op2_a_push_5();
    }
    @name(".op2_t_remove_header_0") table op2_t_remove_header_0 {
        actions = {
            op2_a_remove_header_0;
        }
        default_action = op2_a_remove_header_0();
    }
    @name(".op2_t_remove_header_1") table op2_t_remove_header_1 {
        actions = {
            op2_a_remove_header_1;
        }
        default_action = op2_a_remove_header_1();
    }
    @name(".op2_t_remove_header_2") table op2_t_remove_header_2 {
        actions = {
            op2_a_remove_header_2;
        }
        default_action = op2_a_remove_header_2();
    }
    @name(".op2_t_remove_header_3") table op2_t_remove_header_3 {
        actions = {
            op2_a_remove_header_3;
        }
        default_action = op2_a_remove_header_3();
    }
    @name(".op2_t_remove_header_4") table op2_t_remove_header_4 {
        actions = {
            op2_a_remove_header_4;
        }
        default_action = op2_a_remove_header_4();
    }
    apply {
        if (meta.m.op == 8w0x0) {
        } else {
            if (meta.m.op >> 4 == 8w1) {
                if (meta.m.op & 8w0xf == 8w1) {
                    op2_t_push_1.apply();
                } else {
                    if (meta.m.op & 8w0xf == 8w2) {
                        op2_t_push_2.apply();
                    } else {
                        if (meta.m.op & 8w0xf == 8w3) {
                            op2_t_push_3.apply();
                        } else {
                            if (meta.m.op & 8w0xf == 8w4) {
                                op2_t_push_4.apply();
                            } else {
                                if (meta.m.op & 8w0xf == 8w5) {
                                    op2_t_push_5.apply();
                                }
                            }
                        }
                    }
                }
            } else {
                if (meta.m.op >> 4 == 8w2) {
                    if (meta.m.op & 8w0xf == 8w1) {
                        op2_t_pop_1.apply();
                    } else {
                        if (meta.m.op & 8w0xf == 8w2) {
                            op2_t_pop_2.apply();
                        } else {
                            if (meta.m.op & 8w0xf == 8w3) {
                                op2_t_pop_3.apply();
                            } else {
                                if (meta.m.op & 8w0xf == 8w4) {
                                    op2_t_pop_4.apply();
                                } else {
                                    if (meta.m.op & 8w0xf == 8w5) {
                                        op2_t_pop_5.apply();
                                    }
                                }
                            }
                        }
                    }
                } else {
                    if (meta.m.op >> 4 == 8w3) {
                        if (meta.m.op & 8w0xf == 8w0) {
                            op2_t_assign_header_0.apply();
                        } else {
                            if (meta.m.op & 8w0xf == 8w1) {
                                op2_t_assign_header_1.apply();
                            } else {
                                if (meta.m.op & 8w0xf == 8w2) {
                                    op2_t_assign_header_2.apply();
                                } else {
                                    if (meta.m.op & 8w0xf == 8w3) {
                                        op2_t_assign_header_3.apply();
                                    } else {
                                        if (meta.m.op & 8w0xf == 8w4) {
                                            op2_t_assign_header_4.apply();
                                        }
                                    }
                                }
                            }
                        }
                    } else {
                        if (meta.m.op >> 4 == 8w4) {
                            if (meta.m.op & 8w0xf == 8w0) {
                                op2_t_remove_header_0.apply();
                            } else {
                                if (meta.m.op & 8w0xf == 8w1) {
                                    op2_t_remove_header_1.apply();
                                } else {
                                    if (meta.m.op & 8w0xf == 8w2) {
                                        op2_t_remove_header_2.apply();
                                    } else {
                                        if (meta.m.op & 8w0xf == 8w3) {
                                            op2_t_remove_header_3.apply();
                                        } else {
                                            if (meta.m.op & 8w0xf == 8w4) {
                                                op2_t_remove_header_4.apply();
                                            }
                                        }
                                    }
                                }
                            }
                        } else {
                            if (meta.m.op >> 4 == 8w5) {
                                if (meta.m.op & 8w0xf == 8w0) {
                                    op2_t_add_header_0.apply();
                                } else {
                                    if (meta.m.op & 8w0xf == 8w1) {
                                        op2_t_add_header_1.apply();
                                    } else {
                                        if (meta.m.op & 8w0xf == 8w2) {
                                            op2_t_add_header_2.apply();
                                        } else {
                                            if (meta.m.op & 8w0xf == 8w3) {
                                                op2_t_add_header_3.apply();
                                            } else {
                                                if (meta.m.op & 8w0xf == 8w4) {
                                                    op2_t_add_header_4.apply();
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

control op3_do(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".op3_a_add_header_0") action op3_a_add_header_0() {
        hdr.h2[0].setValid();
    }
    @name(".op3_a_add_header_1") action op3_a_add_header_1() {
        hdr.h2[1].setValid();
    }
    @name(".op3_a_add_header_2") action op3_a_add_header_2() {
        hdr.h2[2].setValid();
    }
    @name(".op3_a_add_header_3") action op3_a_add_header_3() {
        hdr.h2[3].setValid();
    }
    @name(".op3_a_add_header_4") action op3_a_add_header_4() {
        hdr.h2[4].setValid();
    }
    @name(".op3_a_assign_header_0") action op3_a_assign_header_0() {
        hdr.h2[0].hdr_type = 8w2;
        hdr.h2[0].f1 = 8w0xa0;
        hdr.h2[0].f2 = 8w0xa;
        hdr.h2[0].next_hdr_type = 8w9;
    }
    @name(".op3_a_assign_header_1") action op3_a_assign_header_1() {
        hdr.h2[1].hdr_type = 8w2;
        hdr.h2[1].f1 = 8w0xa1;
        hdr.h2[1].f2 = 8w0x1a;
        hdr.h2[1].next_hdr_type = 8w9;
    }
    @name(".op3_a_assign_header_2") action op3_a_assign_header_2() {
        hdr.h2[2].hdr_type = 8w2;
        hdr.h2[2].f1 = 8w0xa2;
        hdr.h2[2].f2 = 8w0x2a;
        hdr.h2[2].next_hdr_type = 8w9;
    }
    @name(".op3_a_assign_header_3") action op3_a_assign_header_3() {
        hdr.h2[3].hdr_type = 8w2;
        hdr.h2[3].f1 = 8w0xa3;
        hdr.h2[3].f2 = 8w0x3a;
        hdr.h2[3].next_hdr_type = 8w9;
    }
    @name(".op3_a_assign_header_4") action op3_a_assign_header_4() {
        hdr.h2[4].hdr_type = 8w2;
        hdr.h2[4].f1 = 8w0xa4;
        hdr.h2[4].f2 = 8w0x4a;
        hdr.h2[4].next_hdr_type = 8w9;
    }
    @name(".op3_a_pop_1") action op3_a_pop_1() {
        hdr.h2.pop_front(1);
    }
    @name(".op3_a_pop_2") action op3_a_pop_2() {
        hdr.h2.pop_front(2);
    }
    @name(".op3_a_pop_3") action op3_a_pop_3() {
        hdr.h2.pop_front(3);
    }
    @name(".op3_a_pop_4") action op3_a_pop_4() {
        hdr.h2.pop_front(4);
    }
    @name(".op3_a_pop_5") action op3_a_pop_5() {
        hdr.h2.pop_front(5);
    }
    @name(".op3_a_push_1") action op3_a_push_1() {
        {
            hdr.h2.push_front(1);
            hdr.h2[0].setValid();
        }
    }
    @name(".op3_a_push_2") action op3_a_push_2() {
        {
            hdr.h2.push_front(2);
            hdr.h2[0].setValid();
            hdr.h2[1].setValid();
        }
    }
    @name(".op3_a_push_3") action op3_a_push_3() {
        {
            hdr.h2.push_front(3);
            hdr.h2[0].setValid();
            hdr.h2[1].setValid();
            hdr.h2[2].setValid();
        }
    }
    @name(".op3_a_push_4") action op3_a_push_4() {
        {
            hdr.h2.push_front(4);
            hdr.h2[0].setValid();
            hdr.h2[1].setValid();
            hdr.h2[2].setValid();
            hdr.h2[3].setValid();
        }
    }
    @name(".op3_a_push_5") action op3_a_push_5() {
        {
            hdr.h2.push_front(5);
            hdr.h2[0].setValid();
            hdr.h2[1].setValid();
            hdr.h2[2].setValid();
            hdr.h2[3].setValid();
            hdr.h2[4].setValid();
        }
    }
    @name(".op3_a_remove_header_0") action op3_a_remove_header_0() {
        hdr.h2[0].setInvalid();
    }
    @name(".op3_a_remove_header_1") action op3_a_remove_header_1() {
        hdr.h2[1].setInvalid();
    }
    @name(".op3_a_remove_header_2") action op3_a_remove_header_2() {
        hdr.h2[2].setInvalid();
    }
    @name(".op3_a_remove_header_3") action op3_a_remove_header_3() {
        hdr.h2[3].setInvalid();
    }
    @name(".op3_a_remove_header_4") action op3_a_remove_header_4() {
        hdr.h2[4].setInvalid();
    }
    @name(".op3_t_add_header_0") table op3_t_add_header_0 {
        actions = {
            op3_a_add_header_0;
        }
        default_action = op3_a_add_header_0();
    }
    @name(".op3_t_add_header_1") table op3_t_add_header_1 {
        actions = {
            op3_a_add_header_1;
        }
        default_action = op3_a_add_header_1();
    }
    @name(".op3_t_add_header_2") table op3_t_add_header_2 {
        actions = {
            op3_a_add_header_2;
        }
        default_action = op3_a_add_header_2();
    }
    @name(".op3_t_add_header_3") table op3_t_add_header_3 {
        actions = {
            op3_a_add_header_3;
        }
        default_action = op3_a_add_header_3();
    }
    @name(".op3_t_add_header_4") table op3_t_add_header_4 {
        actions = {
            op3_a_add_header_4;
        }
        default_action = op3_a_add_header_4();
    }
    @name(".op3_t_assign_header_0") table op3_t_assign_header_0 {
        actions = {
            op3_a_assign_header_0;
        }
        default_action = op3_a_assign_header_0();
    }
    @name(".op3_t_assign_header_1") table op3_t_assign_header_1 {
        actions = {
            op3_a_assign_header_1;
        }
        default_action = op3_a_assign_header_1();
    }
    @name(".op3_t_assign_header_2") table op3_t_assign_header_2 {
        actions = {
            op3_a_assign_header_2;
        }
        default_action = op3_a_assign_header_2();
    }
    @name(".op3_t_assign_header_3") table op3_t_assign_header_3 {
        actions = {
            op3_a_assign_header_3;
        }
        default_action = op3_a_assign_header_3();
    }
    @name(".op3_t_assign_header_4") table op3_t_assign_header_4 {
        actions = {
            op3_a_assign_header_4;
        }
        default_action = op3_a_assign_header_4();
    }
    @name(".op3_t_pop_1") table op3_t_pop_1 {
        actions = {
            op3_a_pop_1;
        }
        default_action = op3_a_pop_1();
    }
    @name(".op3_t_pop_2") table op3_t_pop_2 {
        actions = {
            op3_a_pop_2;
        }
        default_action = op3_a_pop_2();
    }
    @name(".op3_t_pop_3") table op3_t_pop_3 {
        actions = {
            op3_a_pop_3;
        }
        default_action = op3_a_pop_3();
    }
    @name(".op3_t_pop_4") table op3_t_pop_4 {
        actions = {
            op3_a_pop_4;
        }
        default_action = op3_a_pop_4();
    }
    @name(".op3_t_pop_5") table op3_t_pop_5 {
        actions = {
            op3_a_pop_5;
        }
        default_action = op3_a_pop_5();
    }
    @name(".op3_t_push_1") table op3_t_push_1 {
        actions = {
            op3_a_push_1;
        }
        default_action = op3_a_push_1();
    }
    @name(".op3_t_push_2") table op3_t_push_2 {
        actions = {
            op3_a_push_2;
        }
        default_action = op3_a_push_2();
    }
    @name(".op3_t_push_3") table op3_t_push_3 {
        actions = {
            op3_a_push_3;
        }
        default_action = op3_a_push_3();
    }
    @name(".op3_t_push_4") table op3_t_push_4 {
        actions = {
            op3_a_push_4;
        }
        default_action = op3_a_push_4();
    }
    @name(".op3_t_push_5") table op3_t_push_5 {
        actions = {
            op3_a_push_5;
        }
        default_action = op3_a_push_5();
    }
    @name(".op3_t_remove_header_0") table op3_t_remove_header_0 {
        actions = {
            op3_a_remove_header_0;
        }
        default_action = op3_a_remove_header_0();
    }
    @name(".op3_t_remove_header_1") table op3_t_remove_header_1 {
        actions = {
            op3_a_remove_header_1;
        }
        default_action = op3_a_remove_header_1();
    }
    @name(".op3_t_remove_header_2") table op3_t_remove_header_2 {
        actions = {
            op3_a_remove_header_2;
        }
        default_action = op3_a_remove_header_2();
    }
    @name(".op3_t_remove_header_3") table op3_t_remove_header_3 {
        actions = {
            op3_a_remove_header_3;
        }
        default_action = op3_a_remove_header_3();
    }
    @name(".op3_t_remove_header_4") table op3_t_remove_header_4 {
        actions = {
            op3_a_remove_header_4;
        }
        default_action = op3_a_remove_header_4();
    }
    apply {
        if (meta.m.op == 8w0x0) {
        } else {
            if (meta.m.op >> 4 == 8w1) {
                if (meta.m.op & 8w0xf == 8w1) {
                    op3_t_push_1.apply();
                } else {
                    if (meta.m.op & 8w0xf == 8w2) {
                        op3_t_push_2.apply();
                    } else {
                        if (meta.m.op & 8w0xf == 8w3) {
                            op3_t_push_3.apply();
                        } else {
                            if (meta.m.op & 8w0xf == 8w4) {
                                op3_t_push_4.apply();
                            } else {
                                if (meta.m.op & 8w0xf == 8w5) {
                                    op3_t_push_5.apply();
                                }
                            }
                        }
                    }
                }
            } else {
                if (meta.m.op >> 4 == 8w2) {
                    if (meta.m.op & 8w0xf == 8w1) {
                        op3_t_pop_1.apply();
                    } else {
                        if (meta.m.op & 8w0xf == 8w2) {
                            op3_t_pop_2.apply();
                        } else {
                            if (meta.m.op & 8w0xf == 8w3) {
                                op3_t_pop_3.apply();
                            } else {
                                if (meta.m.op & 8w0xf == 8w4) {
                                    op3_t_pop_4.apply();
                                } else {
                                    if (meta.m.op & 8w0xf == 8w5) {
                                        op3_t_pop_5.apply();
                                    }
                                }
                            }
                        }
                    }
                } else {
                    if (meta.m.op >> 4 == 8w3) {
                        if (meta.m.op & 8w0xf == 8w0) {
                            op3_t_assign_header_0.apply();
                        } else {
                            if (meta.m.op & 8w0xf == 8w1) {
                                op3_t_assign_header_1.apply();
                            } else {
                                if (meta.m.op & 8w0xf == 8w2) {
                                    op3_t_assign_header_2.apply();
                                } else {
                                    if (meta.m.op & 8w0xf == 8w3) {
                                        op3_t_assign_header_3.apply();
                                    } else {
                                        if (meta.m.op & 8w0xf == 8w4) {
                                            op3_t_assign_header_4.apply();
                                        }
                                    }
                                }
                            }
                        }
                    } else {
                        if (meta.m.op >> 4 == 8w4) {
                            if (meta.m.op & 8w0xf == 8w0) {
                                op3_t_remove_header_0.apply();
                            } else {
                                if (meta.m.op & 8w0xf == 8w1) {
                                    op3_t_remove_header_1.apply();
                                } else {
                                    if (meta.m.op & 8w0xf == 8w2) {
                                        op3_t_remove_header_2.apply();
                                    } else {
                                        if (meta.m.op & 8w0xf == 8w3) {
                                            op3_t_remove_header_3.apply();
                                        } else {
                                            if (meta.m.op & 8w0xf == 8w4) {
                                                op3_t_remove_header_4.apply();
                                            }
                                        }
                                    }
                                }
                            }
                        } else {
                            if (meta.m.op >> 4 == 8w5) {
                                if (meta.m.op & 8w0xf == 8w0) {
                                    op3_t_add_header_0.apply();
                                } else {
                                    if (meta.m.op & 8w0xf == 8w1) {
                                        op3_t_add_header_1.apply();
                                    } else {
                                        if (meta.m.op & 8w0xf == 8w2) {
                                            op3_t_add_header_2.apply();
                                        } else {
                                            if (meta.m.op & 8w0xf == 8w3) {
                                                op3_t_add_header_3.apply();
                                            } else {
                                                if (meta.m.op & 8w0xf == 8w4) {
                                                    op3_t_add_header_4.apply();
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".op1_a_record_op") action op1_a_record_op() {
        meta.m.op = (bit<8>)hdr.h1.op1;
    }
    @name(".op2_a_record_op") action op2_a_record_op() {
        meta.m.op = (bit<8>)hdr.h1.op2;
    }
    @name(".op3_a_record_op") action op3_a_record_op() {
        meta.m.op = (bit<8>)hdr.h1.op3;
    }
    @name(".a_clear_h2_valid") action a_clear_h2_valid() {
        hdr.h1.h2_valid_bits = 8w0;
    }
    @name(".a_set_h2_valid_bit_0") action a_set_h2_valid_bit_0() {
        hdr.h1.h2_valid_bits[0:0] = 8w0x1[0:0];
    }
    @name(".a_set_h2_valid_bit_1") action a_set_h2_valid_bit_1() {
        hdr.h1.h2_valid_bits[1:1] = 8w0x2[1:1];
    }
    @name(".a_set_h2_valid_bit_2") action a_set_h2_valid_bit_2() {
        hdr.h1.h2_valid_bits[2:2] = 8w0x4[2:2];
    }
    @name(".a_set_h2_valid_bit_3") action a_set_h2_valid_bit_3() {
        hdr.h1.h2_valid_bits[3:3] = 8w0x8[3:3];
    }
    @name(".a_set_h2_valid_bit_4") action a_set_h2_valid_bit_4() {
        hdr.h1.h2_valid_bits[4:4] = 8w0x10[4:4];
    }
    @name(".op1_t_record_op") table op1_t_record_op {
        actions = {
            op1_a_record_op;
        }
        default_action = op1_a_record_op();
    }
    @name(".op2_t_record_op") table op2_t_record_op {
        actions = {
            op2_a_record_op;
        }
        default_action = op2_a_record_op();
    }
    @name(".op3_t_record_op") table op3_t_record_op {
        actions = {
            op3_a_record_op;
        }
        default_action = op3_a_record_op();
    }
    @name(".t_clear_h2_valid") table t_clear_h2_valid {
        actions = {
            a_clear_h2_valid;
        }
        default_action = a_clear_h2_valid();
    }
    @name(".t_set_h2_valid_bit_0") table t_set_h2_valid_bit_0 {
        actions = {
            a_set_h2_valid_bit_0;
        }
        default_action = a_set_h2_valid_bit_0();
    }
    @name(".t_set_h2_valid_bit_1") table t_set_h2_valid_bit_1 {
        actions = {
            a_set_h2_valid_bit_1;
        }
        default_action = a_set_h2_valid_bit_1();
    }
    @name(".t_set_h2_valid_bit_2") table t_set_h2_valid_bit_2 {
        actions = {
            a_set_h2_valid_bit_2;
        }
        default_action = a_set_h2_valid_bit_2();
    }
    @name(".t_set_h2_valid_bit_3") table t_set_h2_valid_bit_3 {
        actions = {
            a_set_h2_valid_bit_3;
        }
        default_action = a_set_h2_valid_bit_3();
    }
    @name(".t_set_h2_valid_bit_4") table t_set_h2_valid_bit_4 {
        actions = {
            a_set_h2_valid_bit_4;
        }
        default_action = a_set_h2_valid_bit_4();
    }
    @name(".op1_do") op1_do() op1_do_0;
    @name(".op2_do") op2_do() op2_do_0;
    @name(".op3_do") op3_do() op3_do_0;
    apply {
        op1_t_record_op.apply();
        op1_do_0.apply(hdr, meta, standard_metadata);
        op2_t_record_op.apply();
        op2_do_0.apply(hdr, meta, standard_metadata);
        op3_t_record_op.apply();
        op3_do_0.apply(hdr, meta, standard_metadata);
        t_clear_h2_valid.apply();
        if (hdr.h2[0].isValid()) {
            t_set_h2_valid_bit_0.apply();
        }
        if (hdr.h2[1].isValid()) {
            t_set_h2_valid_bit_1.apply();
        }
        if (hdr.h2[2].isValid()) {
            t_set_h2_valid_bit_2.apply();
        }
        if (hdr.h2[3].isValid()) {
            t_set_h2_valid_bit_3.apply();
        }
        if (hdr.h2[4].isValid()) {
            t_set_h2_valid_bit_4.apply();
        }
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        packet.emit(hdr.h1);
        packet.emit(hdr.h2);
        packet.emit(hdr.h3);
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

V1Switch(ParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;

