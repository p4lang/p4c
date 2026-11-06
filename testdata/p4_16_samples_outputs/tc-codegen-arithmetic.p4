#include <core.p4>
#include <tc/pna.p4>

typedef bit<32> word_t;
header ethernet_t {
    bit<48> dst;
    bit<48> src;
    bit<16> kind;
}

header values_t {
    bit<8>  op;
    bit<64> x;
    bit<64> y;
    bit<64> result;
}

struct headers_t {
    ethernet_t ethernet;
    values_t   values;
}

struct metadata_t {
    bit<8> unused;
}

bit<64> calculate(in word_t x, in word_t y, in bit<8> op) {
    bit<64> result = 0;
    if (op == 0) {
        result = (x > y - (word_t)14 ? 64w1 : 64w0);
    }
    if (op == 1) {
        result = (x - (word_t)20 != y ? 64w1 : 64w0);
    }
    if (op == 2) {
        result = (bit<64>)(x - (word_t)3);
    }
    if (op == 3) {
        result = (bit<64>)(x - y);
    }
    if (op == 4) {
        result = (bit<64>)(x + (word_t)3);
    }
    if (op == 5) {
        result = (bit<64>)(x + y);
    }
    if (op == 6) {
        result = (bit<64>)(x * y);
    }
    if (op == 7) {
        result = (bit<64>)((x - (word_t)3) * y + (word_t)1);
    }
    if (op == 8) {
        result = (bit<64>)(x + ~(word_t)0);
    }
    return result;
}
parser Parser(packet_in pkt, out headers_t hdr, inout metadata_t meta, in pna_main_parser_input_metadata_t istd) {
    state start {
        pkt.extract(hdr.ethernet);
        verify(hdr.ethernet.kind == 16w0x88b5, error.NoMatch);
        pkt.extract(hdr.values);
        transition accept;
    }
}

control Ingress(inout headers_t hdr, inout metadata_t meta, in pna_main_input_metadata_t istd, inout pna_main_output_metadata_t ostd) {
    action calculate_value() {
        hdr.values.result = calculate((word_t)hdr.values.x, (word_t)hdr.values.y, hdr.values.op);
    }
    apply {
        if (istd.parser_error != error.NoError) {
            drop_packet();
            return;
        }
        hdr.values.result = calculate((word_t)hdr.values.x, (word_t)hdr.values.y, hdr.values.op);
        send_to_port((PortId_t)1);
    }
}

control Deparser(packet_out pkt, inout headers_t hdr, in metadata_t meta, in pna_main_output_metadata_t ostd) {
    apply {
        pkt.emit(hdr.ethernet);
        pkt.emit(hdr.values);
    }
}

PNA_NIC(Parser(), Ingress(), Deparser()) main;
