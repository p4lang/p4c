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

header last_t {
    bit<8> mark;
}

struct headers_t {
    ethernet_t ethernet;
    tail_t     tail;
    last_t     last;
}

struct metadata_t {
    bit<8> seen;
}

struct tuple_0 {
    bit<16> f0;
}

parser Parser(packet_in pkt, out headers_t hdr, inout metadata_t meta, in pna_main_parser_input_metadata_t istd) {
    @name("Parser.tmp") bit<16> tmp;
    @name("Parser.ck") InternetChecksum() ck_0;
    state start {
        pkt.extract<ethernet_t>(hdr.ethernet);
        transition select(hdr.ethernet.kind) {
            16w0x88b5: parse_checksum;
            16w0x88b6: parse_last;
            default: reject;
        }
    }
    state parse_checksum {
        pkt.extract<tail_t>(hdr.tail);
        ck_0.clear();
        ck_0.add<tuple_0>((tuple_0){f0 = hdr.ethernet.kind});
        tmp = ck_0.get();
        meta.seen = (bit<8>)tmp;
        transition parse_last;
    }
    state parse_last {
        pkt.extract<last_t>(hdr.last);
        transition accept;
    }
}

control Ingress(inout headers_t hdr, inout metadata_t meta, in pna_main_input_metadata_t istd, inout pna_main_output_metadata_t ostd) {
    @hidden action tccodegenoutlinechecksum46() {
        send_to_port(32w1);
    }
    @hidden table tbl_tccodegenoutlinechecksum46 {
        actions = {
            tccodegenoutlinechecksum46();
        }
        const default_action = tccodegenoutlinechecksum46();
    }
    apply {
        tbl_tccodegenoutlinechecksum46.apply();
    }
}

control Deparser(packet_out pkt, inout headers_t hdr, in metadata_t meta, in pna_main_output_metadata_t ostd) {
    @hidden action tccodegenoutlinechecksum51() {
        pkt.emit<ethernet_t>(hdr.ethernet);
        pkt.emit<tail_t>(hdr.tail);
        pkt.emit<last_t>(hdr.last);
    }
    @hidden table tbl_tccodegenoutlinechecksum51 {
        actions = {
            tccodegenoutlinechecksum51();
        }
        const default_action = tccodegenoutlinechecksum51();
    }
    apply {
        tbl_tccodegenoutlinechecksum51.apply();
    }
}

PNA_NIC<headers_t, metadata_t>(Parser(), Ingress(), Deparser()) main;
