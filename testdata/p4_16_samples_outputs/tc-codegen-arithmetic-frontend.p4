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

parser Parser(packet_in pkt, out headers_t hdr, inout metadata_t meta, in pna_main_parser_input_metadata_t istd) {
    state start {
        pkt.extract<ethernet_t>(hdr.ethernet);
        verify(hdr.ethernet.kind == 16w0x88b5, error.NoMatch);
        pkt.extract<values_t>(hdr.values);
        transition accept;
    }
}

control Ingress(inout headers_t hdr, inout metadata_t meta, in pna_main_input_metadata_t istd, inout pna_main_output_metadata_t ostd) {
    @name("Ingress.x_0") word_t x_1;
    @name("Ingress.y_0") word_t y_1;
    @name("Ingress.op_0") bit<8> op_1;
    @name("Ingress.retval") bit<64> retval;
    @name("Ingress.tmp") bit<64> tmp;
    @name("Ingress.tmp_0") bit<64> tmp_0;
    @name("Ingress.result") bit<64> result_0;
    @name("Ingress.inlinedRetval") bit<64> inlinedRetval_0;
    apply {
        if (istd.parser_error != error.NoError) {
            drop_packet();
        } else {
            x_1 = (word_t)hdr.values.x;
            y_1 = (word_t)hdr.values.y;
            op_1 = hdr.values.op;
            result_0 = 64w0;
            if (op_1 == 8w0) {
                if (x_1 > y_1 + 32w4294967282) {
                    tmp = 64w1;
                } else {
                    tmp = 64w0;
                }
                result_0 = tmp;
            }
            if (op_1 == 8w1) {
                if (x_1 + 32w4294967276 != y_1) {
                    tmp_0 = 64w1;
                } else {
                    tmp_0 = 64w0;
                }
                result_0 = tmp_0;
            }
            if (op_1 == 8w2) {
                result_0 = (bit<64>)(x_1 + 32w4294967293);
            }
            if (op_1 == 8w3) {
                result_0 = (bit<64>)(x_1 - y_1);
            }
            if (op_1 == 8w4) {
                result_0 = (bit<64>)(x_1 + 32w3);
            }
            if (op_1 == 8w5) {
                result_0 = (bit<64>)(x_1 + y_1);
            }
            if (op_1 == 8w6) {
                result_0 = (bit<64>)(x_1 * y_1);
            }
            if (op_1 == 8w7) {
                result_0 = (bit<64>)((x_1 + 32w4294967293) * y_1 + 32w1);
            }
            if (op_1 == 8w8) {
                result_0 = (bit<64>)(x_1 + 32w4294967295);
            }
            retval = result_0;
            inlinedRetval_0 = retval;
            hdr.values.result = inlinedRetval_0;
            send_to_port((PortId_t)32w1);
        }
    }
}

control Deparser(packet_out pkt, inout headers_t hdr, in metadata_t meta, in pna_main_output_metadata_t ostd) {
    apply {
        pkt.emit<ethernet_t>(hdr.ethernet);
        pkt.emit<values_t>(hdr.values);
    }
}

PNA_NIC<headers_t, metadata_t>(Parser(), Ingress(), Deparser()) main;
