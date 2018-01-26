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

parser subParserImpl(packet_in pkt, inout headers hdr, out bit<8> ret_next_hdr_type) {
    state start {
        pkt.extract<h2_t>(hdr.h2.next);
        verify(hdr.h2.last.hdr_type == 8w2, error.BadHeaderType);
        ret_next_hdr_type = hdr.h2.last.next_hdr_type;
        transition accept;
    }
}

parser parserI(packet_in pkt, out headers hdr, inout metadata meta, inout standard_metadata_t stdmeta) {
    subParserImpl() subp;
    bit<8> my_next_hdr_type;
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
        subp.apply(pkt, hdr, my_next_hdr_type);
        transition select(my_next_hdr_type) {
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
    apply {
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

