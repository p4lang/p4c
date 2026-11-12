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
    bit<8>  seen;
    bit<8>  mark;
    bit<32> left;
}

struct tuple_0 {
    bit<16> f0;
}

struct tuple_1 {
    bit<3>  f0;
    bit<13> f1;
}

parser Parser(packet_in pkt, out headers_t hdr, inout metadata_t meta, in pna_main_parser_input_metadata_t istd) {
    @name("Parser.tmp") bit<16> tmp;
    @name("Parser.tmp_0") bit<16> tmp_0;
    @name("Parser.ck") InternetChecksum() ck_0;
    state start {
        pkt.extract<ethernet_t>(hdr.ethernet);
        meta.left = 32w0;
        ck_0.clear();
        ck_0.add<tuple_0>((tuple_0){f0 = hdr.ethernet.kind});
        tmp = ck_0.get();
        meta.seen = (bit<8>)tmp;
        transition select(hdr.ethernet.kind) {
            16w0x88b5: parse_flags;
            default: reject;
        }
    }
    state parse_flags {
        pkt.extract<flags_t>(hdr.ipv4);
        meta.left = (bit<32>)hdr.ipv4.fragment_offset;
        ck_0.clear();
        ck_0.add<tuple_1>((tuple_1){f0 = hdr.ipv4.flags,f1 = hdr.ipv4.fragment_offset});
        tmp_0 = ck_0.get();
        meta.mark = (bit<8>)tmp_0;
        transition select(hdr.ipv4.flags) {
            3w0: finish;
            default: validate;
        }
    }
    state validate {
        verify(meta.left <= 32w8191, error.ParserInvalidArgument);
        transition finish;
    }
    state finish {
        pkt.extract<tail_t>(hdr.tail);
        transition accept;
    }
}

control Ingress(inout headers_t hdr, inout metadata_t meta, in pna_main_input_metadata_t istd, inout pna_main_output_metadata_t ostd) {
    @hidden action tccodegenoutlinechecksumtwo57() {
        send_to_port(32w1);
    }
    @hidden table tbl_tccodegenoutlinechecksumtwo57 {
        actions = {
            tccodegenoutlinechecksumtwo57();
        }
        const default_action = tccodegenoutlinechecksumtwo57();
    }
    apply {
        tbl_tccodegenoutlinechecksumtwo57.apply();
    }
}

control Deparser(packet_out pkt, inout headers_t hdr, in metadata_t meta, in pna_main_output_metadata_t ostd) {
    @hidden action tccodegenoutlinechecksumtwo62() {
        pkt.emit<ethernet_t>(hdr.ethernet);
        pkt.emit<flags_t>(hdr.ipv4);
        pkt.emit<tail_t>(hdr.tail);
    }
    @hidden table tbl_tccodegenoutlinechecksumtwo62 {
        actions = {
            tccodegenoutlinechecksumtwo62();
        }
        const default_action = tccodegenoutlinechecksumtwo62();
    }
    apply {
        tbl_tccodegenoutlinechecksumtwo62.apply();
    }
}

PNA_NIC<headers_t, metadata_t>(Parser(), Ingress(), Deparser()) main;
