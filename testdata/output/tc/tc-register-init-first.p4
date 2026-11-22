#include <core.p4>
#include <tc/pna.p4>

header ethernet_t {
    bit<48> dst;
    bit<48> src;
    bit<16> etherType;
}

header result_t {
    bit<32> scalar;
    bit<64> wide;
    bit<8>  protocol;
    bit<8>  aux;
}

struct headers_t {
    ethernet_t ethernet;
    result_t   result;
}

struct metadata_t {
}

struct pair_t {
    bit<8> protocol;
    bit<8> aux;
}

parser Parser(packet_in packet, out headers_t hdr, inout metadata_t meta, in pna_main_parser_input_metadata_t istd) {
    state start {
        packet.extract<ethernet_t>(hdr.ethernet);
        packet.extract<result_t>(hdr.result);
        transition accept;
    }
}

control Ingress(inout headers_t hdr, inout metadata_t meta, in pna_main_input_metadata_t istd, inout pna_main_output_metadata_t ostd) {
    Register<bit<32>, bit<32>>(32w10, 32w13) scalar;
    Register<bit<64>, bit<32>>(32w3, 64w0xfedcba9876543210) wide;
    Register<pair_t, bit<32>>(32w3, (pair_t){protocol = 8w17,aux = 8w22}) pair;
    Register<bit<32>, bit<32>>(32w2, 32w0) zero;
    Register<bit<32>, bit<32>>(32w2) unspecified;
    apply {
        bit<32> value = scalar.read(32w1);
        pair_t fields = pair.read(32w1);
        hdr.result.scalar = value;
        hdr.result.wide = wide.read(32w1);
        hdr.result.protocol = fields.protocol;
        hdr.result.aux = fields.aux;
        value = value + 32w10;
        scalar.write(32w1, value);
        unspecified.write(32w1, zero.read(32w1));
        send_to_port(istd.input_port);
    }
}

control Deparser(packet_out packet, inout headers_t hdr, in metadata_t meta, in pna_main_output_metadata_t ostd) {
    apply {
        packet.emit<ethernet_t>(hdr.ethernet);
        packet.emit<result_t>(hdr.result);
    }
}

PNA_NIC<headers_t, metadata_t>(Parser(), Ingress(), Deparser()) main;
