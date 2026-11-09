#include <core.p4>
#include <tc/pna.p4>

header ethernet_t {
    bit<48> dst;
    bit<48> src;
    bit<16> kind;
}

header flags_t {
    bit<3>  flags;
    bit<13> fragment_offset;
}

header tail_t {
    bit<8> value;
}

struct headers_t {
    ethernet_t ethernet;
    flags_t    ipv4;
    tail_t     tail;
}

struct metadata_t {
    bit<32> left;
    bit<32> leftover;
}

parser Parser(packet_in pkt, out headers_t hdr, inout metadata_t meta, in pna_main_parser_input_metadata_t istd) {
    state start {
        pkt.extract<ethernet_t>(hdr.ethernet);
        meta.left = 32w0;
        meta.leftover = 32w1;
        transition select(hdr.ethernet.kind) {
            16w0x88b5: parse_flags;
            default: reject;
        }
    }
    state parse_flags {
        pkt.extract<flags_t>(hdr.ipv4);
        meta.left = (bit<32>)hdr.ipv4.fragment_offset;
        transition select(hdr.ipv4.flags) {
            3w0: finish;
            default: validate;
        }
    }
    state validate {
        verify(meta.left <= 32w8191, error.ParserInvalidArgument);
        meta.leftover = meta.leftover + 32w1;
        transition finish;
    }
    state finish {
        pkt.extract<tail_t>(hdr.tail);
        transition accept;
    }
}

control Ingress(inout headers_t hdr, inout metadata_t meta, in pna_main_input_metadata_t istd, inout pna_main_output_metadata_t ostd) {
    @hidden action pnacodegenemission49() {
        drop_packet();
    }
    @hidden action pnacodegenemission50() {
        drop_packet();
    }
    @hidden action pnacodegenemission51() {
        send_to_port(32w1);
    }
    @hidden table tbl_pnacodegenemission49 {
        actions = {
            pnacodegenemission49();
        }
        const default_action = pnacodegenemission49();
    }
    @hidden table tbl_pnacodegenemission50 {
        actions = {
            pnacodegenemission50();
        }
        const default_action = pnacodegenemission50();
    }
    @hidden table tbl_pnacodegenemission51 {
        actions = {
            pnacodegenemission51();
        }
        const default_action = pnacodegenemission51();
    }
    apply {
        if (istd.parser_error != error.NoError) {
            tbl_pnacodegenemission49.apply();
        } else if (meta.leftover == 32w0) {
            tbl_pnacodegenemission50.apply();
        } else {
            tbl_pnacodegenemission51.apply();
        }
    }
}

control Deparser(packet_out pkt, inout headers_t hdr, in metadata_t meta, in pna_main_output_metadata_t ostd) {
    @hidden action pnacodegenemission56() {
        pkt.emit<ethernet_t>(hdr.ethernet);
        pkt.emit<flags_t>(hdr.ipv4);
        pkt.emit<tail_t>(hdr.tail);
    }
    @hidden table tbl_pnacodegenemission56 {
        actions = {
            pnacodegenemission56();
        }
        const default_action = pnacodegenemission56();
    }
    apply {
        tbl_pnacodegenemission56.apply();
    }
}

PNA_NIC<headers_t, metadata_t>(Parser(), Ingress(), Deparser()) main;
