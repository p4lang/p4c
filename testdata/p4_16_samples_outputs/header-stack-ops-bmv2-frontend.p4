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
    headers tmp_5;
    bit<8> tmp_6;
    headers tmp_7;
    bit<8> tmp_8;
    headers tmp_9;
    bit<8> tmp_10;
    headers hdr_1;
    bit<8> op;
    apply {
        tmp_5 = hdr;
        tmp_6 = hdr.h1.op1;
        hdr_1 = tmp_5;
        op = tmp_6;
        if (op == 8w0x0) 
            ;
        else 
            if (op[7:4] == 4w1) 
                if (op[3:0] == 4w1) 
                    hdr_1.h2.push_front(1);
                else 
                    if (op[3:0] == 4w2) 
                        hdr_1.h2.push_front(2);
                    else 
                        if (op[3:0] == 4w3) 
                            hdr_1.h2.push_front(3);
                        else 
                            if (op[3:0] == 4w4) 
                                hdr_1.h2.push_front(4);
                            else 
                                if (op[3:0] == 4w5) 
                                    hdr_1.h2.push_front(5);
                                else 
                                    if (op[3:0] == 4w6) 
                                        hdr_1.h2.push_front(6);
            else 
                if (op[7:4] == 4w2) 
                    if (op[3:0] == 4w1) 
                        hdr_1.h2.pop_front(1);
                    else 
                        if (op[3:0] == 4w2) 
                            hdr_1.h2.pop_front(2);
                        else 
                            if (op[3:0] == 4w3) 
                                hdr_1.h2.pop_front(3);
                            else 
                                if (op[3:0] == 4w4) 
                                    hdr_1.h2.pop_front(4);
                                else 
                                    if (op[3:0] == 4w5) 
                                        hdr_1.h2.pop_front(5);
                                    else 
                                        if (op[3:0] == 4w6) 
                                            hdr_1.h2.pop_front(6);
                else 
                    if (op[7:4] == 4w3) 
                        if (op[3:0] == 4w0) {
                            hdr_1.h2[0].setValid();
                            hdr_1.h2[0].hdr_type = 8w2;
                            hdr_1.h2[0].f1 = 8w0xa0;
                            hdr_1.h2[0].f2 = 8w0xa;
                            hdr_1.h2[0].next_hdr_type = 8w9;
                        }
                        else 
                            if (op[3:0] == 4w1) {
                                hdr_1.h2[1].setValid();
                                hdr_1.h2[1].hdr_type = 8w2;
                                hdr_1.h2[1].f1 = 8w0xa1;
                                hdr_1.h2[1].f2 = 8w0x1a;
                                hdr_1.h2[1].next_hdr_type = 8w9;
                            }
                            else 
                                if (op[3:0] == 4w2) {
                                    hdr_1.h2[2].setValid();
                                    hdr_1.h2[2].hdr_type = 8w2;
                                    hdr_1.h2[2].f1 = 8w0xa2;
                                    hdr_1.h2[2].f2 = 8w0x2a;
                                    hdr_1.h2[2].next_hdr_type = 8w9;
                                }
                                else 
                                    if (op[3:0] == 4w3) {
                                        hdr_1.h2[3].setValid();
                                        hdr_1.h2[3].hdr_type = 8w2;
                                        hdr_1.h2[3].f1 = 8w0xa3;
                                        hdr_1.h2[3].f2 = 8w0x3a;
                                        hdr_1.h2[3].next_hdr_type = 8w9;
                                    }
                                    else 
                                        if (op[3:0] == 4w4) {
                                            hdr_1.h2[4].setValid();
                                            hdr_1.h2[4].hdr_type = 8w2;
                                            hdr_1.h2[4].f1 = 8w0xa4;
                                            hdr_1.h2[4].f2 = 8w0x4a;
                                            hdr_1.h2[4].next_hdr_type = 8w9;
                                        }
                    else 
                        if (op[7:4] == 4w4) 
                            if (op[3:0] == 4w0) 
                                hdr_1.h2[0].setInvalid();
                            else 
                                if (op[3:0] == 4w1) 
                                    hdr_1.h2[1].setInvalid();
                                else 
                                    if (op[3:0] == 4w2) 
                                        hdr_1.h2[2].setInvalid();
                                    else 
                                        if (op[3:0] == 4w3) 
                                            hdr_1.h2[3].setInvalid();
                                        else 
                                            if (op[3:0] == 4w4) 
                                                hdr_1.h2[4].setInvalid();
        tmp_5 = hdr_1;
        hdr = tmp_5;
        tmp_7 = hdr;
        tmp_8 = hdr.h1.op2;
        hdr_1 = tmp_7;
        op = tmp_8;
        if (op == 8w0x0) 
            ;
        else 
            if (op[7:4] == 4w1) 
                if (op[3:0] == 4w1) 
                    hdr_1.h2.push_front(1);
                else 
                    if (op[3:0] == 4w2) 
                        hdr_1.h2.push_front(2);
                    else 
                        if (op[3:0] == 4w3) 
                            hdr_1.h2.push_front(3);
                        else 
                            if (op[3:0] == 4w4) 
                                hdr_1.h2.push_front(4);
                            else 
                                if (op[3:0] == 4w5) 
                                    hdr_1.h2.push_front(5);
                                else 
                                    if (op[3:0] == 4w6) 
                                        hdr_1.h2.push_front(6);
            else 
                if (op[7:4] == 4w2) 
                    if (op[3:0] == 4w1) 
                        hdr_1.h2.pop_front(1);
                    else 
                        if (op[3:0] == 4w2) 
                            hdr_1.h2.pop_front(2);
                        else 
                            if (op[3:0] == 4w3) 
                                hdr_1.h2.pop_front(3);
                            else 
                                if (op[3:0] == 4w4) 
                                    hdr_1.h2.pop_front(4);
                                else 
                                    if (op[3:0] == 4w5) 
                                        hdr_1.h2.pop_front(5);
                                    else 
                                        if (op[3:0] == 4w6) 
                                            hdr_1.h2.pop_front(6);
                else 
                    if (op[7:4] == 4w3) 
                        if (op[3:0] == 4w0) {
                            hdr_1.h2[0].setValid();
                            hdr_1.h2[0].hdr_type = 8w2;
                            hdr_1.h2[0].f1 = 8w0xa0;
                            hdr_1.h2[0].f2 = 8w0xa;
                            hdr_1.h2[0].next_hdr_type = 8w9;
                        }
                        else 
                            if (op[3:0] == 4w1) {
                                hdr_1.h2[1].setValid();
                                hdr_1.h2[1].hdr_type = 8w2;
                                hdr_1.h2[1].f1 = 8w0xa1;
                                hdr_1.h2[1].f2 = 8w0x1a;
                                hdr_1.h2[1].next_hdr_type = 8w9;
                            }
                            else 
                                if (op[3:0] == 4w2) {
                                    hdr_1.h2[2].setValid();
                                    hdr_1.h2[2].hdr_type = 8w2;
                                    hdr_1.h2[2].f1 = 8w0xa2;
                                    hdr_1.h2[2].f2 = 8w0x2a;
                                    hdr_1.h2[2].next_hdr_type = 8w9;
                                }
                                else 
                                    if (op[3:0] == 4w3) {
                                        hdr_1.h2[3].setValid();
                                        hdr_1.h2[3].hdr_type = 8w2;
                                        hdr_1.h2[3].f1 = 8w0xa3;
                                        hdr_1.h2[3].f2 = 8w0x3a;
                                        hdr_1.h2[3].next_hdr_type = 8w9;
                                    }
                                    else 
                                        if (op[3:0] == 4w4) {
                                            hdr_1.h2[4].setValid();
                                            hdr_1.h2[4].hdr_type = 8w2;
                                            hdr_1.h2[4].f1 = 8w0xa4;
                                            hdr_1.h2[4].f2 = 8w0x4a;
                                            hdr_1.h2[4].next_hdr_type = 8w9;
                                        }
                    else 
                        if (op[7:4] == 4w4) 
                            if (op[3:0] == 4w0) 
                                hdr_1.h2[0].setInvalid();
                            else 
                                if (op[3:0] == 4w1) 
                                    hdr_1.h2[1].setInvalid();
                                else 
                                    if (op[3:0] == 4w2) 
                                        hdr_1.h2[2].setInvalid();
                                    else 
                                        if (op[3:0] == 4w3) 
                                            hdr_1.h2[3].setInvalid();
                                        else 
                                            if (op[3:0] == 4w4) 
                                                hdr_1.h2[4].setInvalid();
        tmp_7 = hdr_1;
        hdr = tmp_7;
        tmp_9 = hdr;
        tmp_10 = hdr.h1.op3;
        hdr_1 = tmp_9;
        op = tmp_10;
        if (op == 8w0x0) 
            ;
        else 
            if (op[7:4] == 4w1) 
                if (op[3:0] == 4w1) 
                    hdr_1.h2.push_front(1);
                else 
                    if (op[3:0] == 4w2) 
                        hdr_1.h2.push_front(2);
                    else 
                        if (op[3:0] == 4w3) 
                            hdr_1.h2.push_front(3);
                        else 
                            if (op[3:0] == 4w4) 
                                hdr_1.h2.push_front(4);
                            else 
                                if (op[3:0] == 4w5) 
                                    hdr_1.h2.push_front(5);
                                else 
                                    if (op[3:0] == 4w6) 
                                        hdr_1.h2.push_front(6);
            else 
                if (op[7:4] == 4w2) 
                    if (op[3:0] == 4w1) 
                        hdr_1.h2.pop_front(1);
                    else 
                        if (op[3:0] == 4w2) 
                            hdr_1.h2.pop_front(2);
                        else 
                            if (op[3:0] == 4w3) 
                                hdr_1.h2.pop_front(3);
                            else 
                                if (op[3:0] == 4w4) 
                                    hdr_1.h2.pop_front(4);
                                else 
                                    if (op[3:0] == 4w5) 
                                        hdr_1.h2.pop_front(5);
                                    else 
                                        if (op[3:0] == 4w6) 
                                            hdr_1.h2.pop_front(6);
                else 
                    if (op[7:4] == 4w3) 
                        if (op[3:0] == 4w0) {
                            hdr_1.h2[0].setValid();
                            hdr_1.h2[0].hdr_type = 8w2;
                            hdr_1.h2[0].f1 = 8w0xa0;
                            hdr_1.h2[0].f2 = 8w0xa;
                            hdr_1.h2[0].next_hdr_type = 8w9;
                        }
                        else 
                            if (op[3:0] == 4w1) {
                                hdr_1.h2[1].setValid();
                                hdr_1.h2[1].hdr_type = 8w2;
                                hdr_1.h2[1].f1 = 8w0xa1;
                                hdr_1.h2[1].f2 = 8w0x1a;
                                hdr_1.h2[1].next_hdr_type = 8w9;
                            }
                            else 
                                if (op[3:0] == 4w2) {
                                    hdr_1.h2[2].setValid();
                                    hdr_1.h2[2].hdr_type = 8w2;
                                    hdr_1.h2[2].f1 = 8w0xa2;
                                    hdr_1.h2[2].f2 = 8w0x2a;
                                    hdr_1.h2[2].next_hdr_type = 8w9;
                                }
                                else 
                                    if (op[3:0] == 4w3) {
                                        hdr_1.h2[3].setValid();
                                        hdr_1.h2[3].hdr_type = 8w2;
                                        hdr_1.h2[3].f1 = 8w0xa3;
                                        hdr_1.h2[3].f2 = 8w0x3a;
                                        hdr_1.h2[3].next_hdr_type = 8w9;
                                    }
                                    else 
                                        if (op[3:0] == 4w4) {
                                            hdr_1.h2[4].setValid();
                                            hdr_1.h2[4].hdr_type = 8w2;
                                            hdr_1.h2[4].f1 = 8w0xa4;
                                            hdr_1.h2[4].f2 = 8w0x4a;
                                            hdr_1.h2[4].next_hdr_type = 8w9;
                                        }
                    else 
                        if (op[7:4] == 4w4) 
                            if (op[3:0] == 4w0) 
                                hdr_1.h2[0].setInvalid();
                            else 
                                if (op[3:0] == 4w1) 
                                    hdr_1.h2[1].setInvalid();
                                else 
                                    if (op[3:0] == 4w2) 
                                        hdr_1.h2[2].setInvalid();
                                    else 
                                        if (op[3:0] == 4w3) 
                                            hdr_1.h2[3].setInvalid();
                                        else 
                                            if (op[3:0] == 4w4) 
                                                hdr_1.h2[4].setInvalid();
        tmp_9 = hdr_1;
        hdr = tmp_9;
        hdr.h1.h2_valid_bits = 8w0;
        if (hdr.h2[0].isValid()) 
            hdr.h1.h2_valid_bits[0:0] = 1w1;
        if (hdr.h2[1].isValid()) 
            hdr.h1.h2_valid_bits[1:1] = 1w1;
        if (hdr.h2[2].isValid()) 
            hdr.h1.h2_valid_bits[2:2] = 1w1;
        if (hdr.h2[3].isValid()) 
            hdr.h1.h2_valid_bits[3:3] = 1w1;
        if (hdr.h2[4].isValid()) 
            hdr.h1.h2_valid_bits[4:4] = 1w1;
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
        packet.emit<h2_t[5]>(hdr.h2);
        packet.emit<h3_t>(hdr.h3);
    }
}

V1Switch<headers, metadata>(parserI(), vc(), cIngress(), cEgress(), uc(), DeparserI()) main;

