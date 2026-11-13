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

struct tuple_0 {
    bit<16> f0;
}

struct tuple_1 {
    bit<3>  f0;
    bit<13> f1;
}

struct tuple_2 {
    bit<8> f0;
    bit<8> f1;
}

struct tuple_3 {
    bit<128> f0;
}

parser Parser(packet_in pkt, out headers_t hdr, inout metadata_t meta, in pna_main_parser_input_metadata_t istd) {
    @name("Parser.ck") InternetChecksum() ck_0;
    state start {
        pkt.extract<ethernet_t>(hdr.ethernet);
        ck_0.clear();
        ck_0.add<tuple_0>((tuple_0){f0 = hdr.ethernet.kind});
        transition select(hdr.ethernet.kind) {
            16w0x88b5: parse_flags;
            default: reject;
        }
    }
    state parse_flags {
        pkt.extract<flags_t>(hdr.ipv4);
        ck_0.add<tuple_1>((tuple_1){f0 = hdr.ipv4.flags,f1 = hdr.ipv4.fragment_offset});
        pkt.extract<pair_t>(hdr.pair);
        ck_0.add<tuple_2>((tuple_2){f0 = hdr.pair.high,f1 = hdr.pair.low});
        pkt.extract<wide_t>(hdr.wide);
        ck_0.add<tuple_3>((tuple_3){f0 = hdr.wide.value});
        meta.sum = ck_0.get();
        meta.seen = 8w1;
        transition accept;
    }
}

control Ingress(inout headers_t hdr, inout metadata_t meta, in pna_main_input_metadata_t istd, inout pna_main_output_metadata_t ostd) {
    @hidden action tccodegenchecksumbits56() {
        drop_packet();
    }
    @hidden action tccodegenchecksumbits57() {
        drop_packet();
    }
    @hidden action tccodegenchecksumbits58() {
        send_to_port(32w1);
    }
    @hidden table tbl_tccodegenchecksumbits56 {
        actions = {
            tccodegenchecksumbits56();
        }
        const default_action = tccodegenchecksumbits56();
    }
    @hidden table tbl_tccodegenchecksumbits57 {
        actions = {
            tccodegenchecksumbits57();
        }
        const default_action = tccodegenchecksumbits57();
    }
    @hidden table tbl_tccodegenchecksumbits58 {
        actions = {
            tccodegenchecksumbits58();
        }
        const default_action = tccodegenchecksumbits58();
    }
    apply {
        if (istd.parser_error != error.NoError) {
            tbl_tccodegenchecksumbits56.apply();
        } else if (meta.seen == 8w0) {
            tbl_tccodegenchecksumbits57.apply();
        } else {
            tbl_tccodegenchecksumbits58.apply();
        }
    }
}

control Deparser(packet_out pkt, inout headers_t hdr, in metadata_t meta, in pna_main_output_metadata_t ostd) {
    @hidden action tccodegenchecksumbits64() {
        pkt.emit<ethernet_t>(hdr.ethernet);
        pkt.emit<flags_t>(hdr.ipv4);
        pkt.emit<pair_t>(hdr.pair);
        pkt.emit<wide_t>(hdr.wide);
    }
    @hidden table tbl_tccodegenchecksumbits64 {
        actions = {
            tccodegenchecksumbits64();
        }
        const default_action = tccodegenchecksumbits64();
    }
    apply {
        tbl_tccodegenchecksumbits64.apply();
    }
}

PNA_NIC<headers_t, metadata_t>(Parser(), Ingress(), Deparser()) main;
