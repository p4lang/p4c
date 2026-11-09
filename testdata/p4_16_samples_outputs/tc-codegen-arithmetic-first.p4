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
    bit<64> result = 64w0;
    if (op == 8w0) {
        result = (x > y + 32w4294967282 ? 64w1 : 64w0);
    }
    if (op == 8w1) {
        result = (x + 32w4294967276 != y ? 64w1 : 64w0);
    }
    if (op == 8w2) {
        result = (bit<64>)(x + 32w4294967293);
    }
    if (op == 8w3) {
        result = (bit<64>)(x - y);
    }
    if (op == 8w4) {
        result = (bit<64>)(x + 32w3);
    }
    if (op == 8w5) {
        result = (bit<64>)(x + y);
    }
    if (op == 8w6) {
        result = (bit<64>)(x * y);
    }
    if (op == 8w7) {
        result = (bit<64>)((x + 32w4294967293) * y + 32w1);
    }
    if (op == 8w8) {
        result = (bit<64>)(x + 32w4294967295);
    }
    return result;
}
parser Parser(packet_in pkt, out headers_t hdr, inout metadata_t meta, in pna_main_parser_input_metadata_t istd) {
    state start {
        pkt.extract<ethernet_t>(hdr.ethernet);
        verify(hdr.ethernet.kind == 16w0x88b5, error.NoMatch);
        pkt.extract<values_t>(hdr.values);
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
        send_to_port((PortId_t)32w1);
    }
}

control Deparser(packet_out pkt, inout headers_t hdr, in metadata_t meta, in pna_main_output_metadata_t ostd) {
    apply {
        pkt.emit<ethernet_t>(hdr.ethernet);
        pkt.emit<values_t>(hdr.values);
    }
}

PNA_NIC<headers_t, metadata_t>(Parser(), Ingress(), Deparser()) main;
