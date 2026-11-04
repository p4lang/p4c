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
    @name("Ingress.value") bit<32> value_0;
    @name("Ingress.fields") pair_t fields_0;
    @name("Ingress.tmp") bit<32> tmp;
    @name("Ingress.scalar") Register<bit<32>, bit<32>>(32w10, 32w13) scalar_0;
    @name("Ingress.wide") Register<bit<64>, bit<32>>(32w3, 64w0xfedcba9876543210) wide_0;
    @name("Ingress.pair") Register<pair_t, bit<32>>(32w3, (pair_t){protocol = 8w17,aux = 8w22}) pair_0;
    @name("Ingress.zero") Register<bit<32>, bit<32>>(32w2, 32w0) zero_0;
    @name("Ingress.unspecified") Register<bit<32>, bit<32>>(32w2) unspecified_0;
    @hidden action pnaregisterinit48() {
        value_0 = scalar_0.read(32w1);
        fields_0 = pair_0.read(32w1);
        hdr.result.scalar = value_0;
        hdr.result.wide = wide_0.read(32w1);
        hdr.result.protocol = fields_0.protocol;
        hdr.result.aux = fields_0.aux;
        value_0 = value_0 + 32w10;
        scalar_0.write(32w1, value_0);
        tmp = zero_0.read(32w1);
        unspecified_0.write(32w1, tmp);
        send_to_port(istd.input_port);
    }
    @hidden table tbl_pnaregisterinit48 {
        actions = {
            pnaregisterinit48();
        }
        const default_action = pnaregisterinit48();
    }
    apply {
        tbl_pnaregisterinit48.apply();
    }
}

control Deparser(packet_out packet, inout headers_t hdr, in metadata_t meta, in pna_main_output_metadata_t ostd) {
    @hidden action pnaregisterinit64() {
        packet.emit<ethernet_t>(hdr.ethernet);
        packet.emit<result_t>(hdr.result);
    }
    @hidden table tbl_pnaregisterinit64 {
        actions = {
            pnaregisterinit64();
        }
        const default_action = pnaregisterinit64();
    }
    apply {
        tbl_pnaregisterinit64.apply();
    }
}

PNA_NIC<headers_t, metadata_t>(Parser(), Ingress(), Deparser()) main;
