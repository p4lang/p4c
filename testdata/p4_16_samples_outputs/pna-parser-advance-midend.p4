#include <core.p4>
#include <tc/pna.p4>

header ethernet_t {
    bit<48> dst;
    bit<48> src;
    bit<16> ether_type;
}

header prefix_t {
    bit<32> bits;
}

header last_t {
    bit<8> value;
}

struct headers_t {
    ethernet_t ethernet;
    prefix_t   prefix;
    last_t     last;
}

struct metadata_t {
    bit<8> unused;
}

parser Parser(packet_in pkt, out headers_t hdr, inout metadata_t meta, in pna_main_parser_input_metadata_t istd) {
    state start {
        pkt.extract<ethernet_t>(hdr.ethernet);
        pkt.extract<prefix_t>(hdr.prefix);
        verify(hdr.prefix.bits <= 32w11968, error.ParserInvalidArgument);
        pkt.advance(hdr.prefix.bits);
        pkt.advance(32w8);
        pkt.extract<last_t>(hdr.last);
        transition accept;
    }
}

control Ingress(inout headers_t hdr, inout metadata_t meta, in pna_main_input_metadata_t istd, inout pna_main_output_metadata_t ostd) {
    @hidden action pnaparseradvance31() {
        drop_packet();
    }
    @hidden action pnaparseradvance32() {
        send_to_port(istd.input_port);
    }
    @hidden table tbl_pnaparseradvance31 {
        actions = {
            pnaparseradvance31();
        }
        const default_action = pnaparseradvance31();
    }
    @hidden table tbl_pnaparseradvance32 {
        actions = {
            pnaparseradvance32();
        }
        const default_action = pnaparseradvance32();
    }
    apply {
        if (istd.parser_error != error.NoError) {
            tbl_pnaparseradvance31.apply();
        } else {
            tbl_pnaparseradvance32.apply();
        }
    }
}

control Deparser(packet_out pkt, inout headers_t hdr, in metadata_t meta, in pna_main_output_metadata_t ostd) {
    @hidden action pnaparseradvance38() {
        pkt.emit<ethernet_t>(hdr.ethernet);
        pkt.emit<prefix_t>(hdr.prefix);
        pkt.emit<last_t>(hdr.last);
    }
    @hidden table tbl_pnaparseradvance38 {
        actions = {
            pnaparseradvance38();
        }
        const default_action = pnaparseradvance38();
    }
    apply {
        tbl_pnaparseradvance38.apply();
    }
}

PNA_NIC<headers_t, metadata_t>(Parser(), Ingress(), Deparser()) main;
