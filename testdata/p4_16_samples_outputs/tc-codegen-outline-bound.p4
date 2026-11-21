#include <core.p4>
#include <tc/pna.p4>

header ethernet_t {
    bit<48> dst;
    bit<48> src;
    bit<16> kind;
}

header tail_t {
    bit<8> value;
}

struct headers_t {
    ethernet_t ethernet;
    tail_t     tail;
}

struct metadata_t {
    bit<32> skip;
    bit<32> seen;
}

parser Parser(packet_in pkt, out headers_t hdr, inout metadata_t meta, in pna_main_parser_input_metadata_t istd) {
    state start {
        pkt.extract(hdr.ethernet);
        meta.skip = 8;
        transition select(hdr.ethernet.kind) {
            0x88b5: walk;
            default: reject;
        }
    }
    state walk {
        verify(meta.skip <= 64, error.ParserInvalidArgument);
        pkt.advance(meta.skip);
        meta.seen = 1;
        transition select(hdr.ethernet.kind) {
            0x88b5: finish;
            default: reject;
        }
    }
    state finish {
        pkt.extract(hdr.tail);
        transition accept;
    }
}

control Ingress(inout headers_t hdr, inout metadata_t meta, in pna_main_input_metadata_t istd, inout pna_main_output_metadata_t ostd) {
    apply {
        if (istd.parser_error != error.NoError) {
            drop_packet();
            return;
        }
        if (meta.seen == 0) {
            drop_packet();
            return;
        }
        send_to_port((PortId_t)1);
    }
}

control Deparser(packet_out pkt, inout headers_t hdr, in metadata_t meta, in pna_main_output_metadata_t ostd) {
    apply {
        pkt.emit(hdr.ethernet);
        pkt.emit(hdr.tail);
    }
}

PNA_NIC(Parser(), Ingress(), Deparser()) main;
