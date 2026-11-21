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

parser Parser(packet_in pkt, out headers_t pkts, inout metadata_t md, in pna_main_parser_input_metadata_t istd) {
    state start {
        pkt.extract<ethernet_t>(pkts.ethernet);
        md.left = 32w0;
        md.leftover = 32w1;
        transition select(pkts.ethernet.kind) {
            16w0x88b5: parse_flags;
            default: reject;
        }
    }
    state parse_flags {
        pkt.extract<flags_t>(pkts.ipv4);
        md.left = (bit<32>)pkts.ipv4.fragment_offset;
        transition select(pkts.ipv4.flags) {
            3w0: finish;
            default: validate;
        }
    }
    state validate {
        verify(md.left <= 32w8191, error.ParserInvalidArgument);
        md.leftover = md.leftover + 32w1;
        transition finish;
    }
    state finish {
        pkt.extract<tail_t>(pkts.tail);
        transition accept;
    }
}

control Ingress(inout headers_t pkts, inout metadata_t md, in pna_main_input_metadata_t istd, inout pna_main_output_metadata_t ostd) {
    @hidden action tccodegenoutlinerenamed47() {
        drop_packet();
    }
    @hidden action tccodegenoutlinerenamed48() {
        drop_packet();
    }
    @hidden action tccodegenoutlinerenamed49() {
        send_to_port(istd.input_port);
    }
    @hidden table tbl_tccodegenoutlinerenamed47 {
        actions = {
            tccodegenoutlinerenamed47();
        }
        const default_action = tccodegenoutlinerenamed47();
    }
    @hidden table tbl_tccodegenoutlinerenamed48 {
        actions = {
            tccodegenoutlinerenamed48();
        }
        const default_action = tccodegenoutlinerenamed48();
    }
    @hidden table tbl_tccodegenoutlinerenamed49 {
        actions = {
            tccodegenoutlinerenamed49();
        }
        const default_action = tccodegenoutlinerenamed49();
    }
    apply {
        if (istd.parser_error != error.NoError) {
            tbl_tccodegenoutlinerenamed47.apply();
        } else if (md.leftover == 32w0) {
            tbl_tccodegenoutlinerenamed48.apply();
        } else {
            tbl_tccodegenoutlinerenamed49.apply();
        }
    }
}

control Deparser(packet_out pkt, inout headers_t pkts, in metadata_t md, in pna_main_output_metadata_t ostd) {
    @hidden action tccodegenoutlinerenamed54() {
        pkt.emit<ethernet_t>(pkts.ethernet);
        pkt.emit<flags_t>(pkts.ipv4);
        pkt.emit<tail_t>(pkts.tail);
    }
    @hidden table tbl_tccodegenoutlinerenamed54 {
        actions = {
            tccodegenoutlinerenamed54();
        }
        const default_action = tccodegenoutlinerenamed54();
    }
    apply {
        tbl_tccodegenoutlinerenamed54.apply();
    }
}

PNA_NIC<headers_t, metadata_t>(Parser(), Ingress(), Deparser()) main;
