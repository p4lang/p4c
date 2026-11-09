#include <core.p4>
#include <tc/pna.p4>

header ethernet_t {
    bit<48> dst;
    bit<48> src;
    bit<16> kind;
}

struct headers_t {
    ethernet_t ethernet;
}

struct metadata_t {
    bit<8> unused;
}

parser Parser(packet_in pkt, out headers_t hdr, inout metadata_t meta, in pna_main_parser_input_metadata_t istd) {
    state start {
        pkt.extract<ethernet_t>(hdr.ethernet);
        transition accept;
    }
}

control Ingress(inout headers_t hdr, inout metadata_t meta, in pna_main_input_metadata_t istd, inout pna_main_output_metadata_t ostd) {
    @name("Ingress.counters") Counter<bit<64>, bit<32>>(32w8, PNA_CounterType_t.PACKETS) counters_0;
    @hidden action tccodegencounter22() {
        drop_packet();
    }
    @hidden action tccodegencounter27() {
        counters_0.count((bit<32>)hdr.ethernet.kind & 32w7);
        counters_0.count((bit<32>)hdr.ethernet.kind >> 1);
    }
    @hidden action tccodegencounter23() {
        counters_0.count(32w0);
        counters_0.count(32w7);
        counters_0.count((bit<32>)(hdr.ethernet.kind & 16w7));
    }
    @hidden action tccodegencounter30() {
        send_to_port(istd.input_port);
    }
    @hidden table tbl_tccodegencounter22 {
        actions = {
            tccodegencounter22();
        }
        const default_action = tccodegencounter22();
    }
    @hidden table tbl_tccodegencounter23 {
        actions = {
            tccodegencounter23();
        }
        const default_action = tccodegencounter23();
    }
    @hidden table tbl_tccodegencounter27 {
        actions = {
            tccodegencounter27();
        }
        const default_action = tccodegencounter27();
    }
    @hidden table tbl_tccodegencounter30 {
        actions = {
            tccodegencounter30();
        }
        const default_action = tccodegencounter30();
    }
    apply {
        if (istd.parser_error != error.NoError) {
            tbl_tccodegencounter22.apply();
        } else {
            tbl_tccodegencounter23.apply();
            if (hdr.ethernet.kind != 16w0) {
                tbl_tccodegencounter27.apply();
            }
            tbl_tccodegencounter30.apply();
        }
    }
}

control Deparser(packet_out pkt, inout headers_t hdr, in metadata_t meta, in pna_main_output_metadata_t ostd) {
    @hidden action tccodegencounter35() {
        pkt.emit<ethernet_t>(hdr.ethernet);
    }
    @hidden table tbl_tccodegencounter35 {
        actions = {
            tccodegencounter35();
        }
        const default_action = tccodegencounter35();
    }
    apply {
        tbl_tccodegencounter35.apply();
    }
}

PNA_NIC<headers_t, metadata_t>(Parser(), Ingress(), Deparser()) main;
