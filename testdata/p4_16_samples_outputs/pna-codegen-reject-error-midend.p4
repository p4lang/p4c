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
        verify(meta.left == 32w0, error.ParserInvalidArgument);
        meta.leftover = meta.leftover + 32w1;
        transition finish;
    }
    state finish {
        pkt.extract<tail_t>(hdr.tail);
        transition accept;
    }
}

control Ingress(inout headers_t hdr, inout metadata_t meta, in pna_main_input_metadata_t istd, inout pna_main_output_metadata_t ostd) {
    @hidden action pnacodegenrejecterror45() {
        drop_packet();
    }
    @hidden action pnacodegenrejecterror46() {
        drop_packet();
    }
    @hidden action pnacodegenrejecterror47() {
        send_to_port(32w1);
    }
    @hidden table tbl_pnacodegenrejecterror45 {
        actions = {
            pnacodegenrejecterror45();
        }
        const default_action = pnacodegenrejecterror45();
    }
    @hidden table tbl_pnacodegenrejecterror46 {
        actions = {
            pnacodegenrejecterror46();
        }
        const default_action = pnacodegenrejecterror46();
    }
    @hidden table tbl_pnacodegenrejecterror47 {
        actions = {
            pnacodegenrejecterror47();
        }
        const default_action = pnacodegenrejecterror47();
    }
    apply {
        if (istd.parser_error != error.NoError) {
            tbl_pnacodegenrejecterror45.apply();
        } else if (meta.leftover == 32w0) {
            tbl_pnacodegenrejecterror46.apply();
        } else {
            tbl_pnacodegenrejecterror47.apply();
        }
    }
}

control Deparser(packet_out pkt, inout headers_t hdr, in metadata_t meta, in pna_main_output_metadata_t ostd) {
    @hidden action pnacodegenrejecterror52() {
        pkt.emit<ethernet_t>(hdr.ethernet);
        pkt.emit<flags_t>(hdr.ipv4);
        pkt.emit<tail_t>(hdr.tail);
    }
    @hidden table tbl_pnacodegenrejecterror52 {
        actions = {
            pnacodegenrejecterror52();
        }
        const default_action = pnacodegenrejecterror52();
    }
    apply {
        tbl_pnacodegenrejecterror52.apply();
    }
}

PNA_NIC<headers_t, metadata_t>(Parser(), Ingress(), Deparser()) main;
