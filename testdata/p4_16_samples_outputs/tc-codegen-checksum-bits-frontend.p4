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

header pair_t {
    bit<8> high;
    bit<8> low;
}

header wide_t {
    bit<128> value;
}

struct headers_t {
    ethernet_t ethernet;
    flags_t    ipv4;
    pair_t     pair;
    wide_t     wide;
}

struct metadata_t {
    bit<8>  seen;
    bit<16> sum;
}

parser Parser(packet_in pkt, out headers_t hdr, inout metadata_t meta, in pna_main_parser_input_metadata_t istd) {
    @name("Parser.ck") InternetChecksum() ck_0;
    state start {
        pkt.extract<ethernet_t>(hdr.ethernet);
        ck_0.clear();
        ck_0.add<tuple<bit<16>>>({ hdr.ethernet.kind });
        transition select(hdr.ethernet.kind) {
            16w0x88b5: parse_flags;
            default: reject;
        }
    }
    state parse_flags {
        pkt.extract<flags_t>(hdr.ipv4);
        ck_0.add<tuple<bit<3>, bit<13>>>({ hdr.ipv4.flags, hdr.ipv4.fragment_offset });
        pkt.extract<pair_t>(hdr.pair);
        ck_0.add<tuple<bit<8>, bit<8>>>({ hdr.pair.high, hdr.pair.low });
        pkt.extract<wide_t>(hdr.wide);
        ck_0.add<tuple<bit<128>>>({ hdr.wide.value });
        meta.sum = ck_0.get();
        meta.seen = 8w1;
        transition accept;
    }
}

control Ingress(inout headers_t hdr, inout metadata_t meta, in pna_main_input_metadata_t istd, inout pna_main_output_metadata_t ostd) {
    apply {
        if (istd.parser_error != error.NoError) {
            drop_packet();
        } else if (meta.seen == 8w0) {
            drop_packet();
        } else {
            send_to_port((PortId_t)32w1);
        }
    }
}

control Deparser(packet_out pkt, inout headers_t hdr, in metadata_t meta, in pna_main_output_metadata_t ostd) {
    apply {
        pkt.emit<ethernet_t>(hdr.ethernet);
        pkt.emit<flags_t>(hdr.ipv4);
        pkt.emit<pair_t>(hdr.pair);
        pkt.emit<wide_t>(hdr.wide);
    }
}

PNA_NIC<headers_t, metadata_t>(Parser(), Ingress(), Deparser()) main;
