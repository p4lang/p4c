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
    @hidden action pnacodegencounter22() {
        drop_packet();
    }
    @hidden action pnacodegencounter27() {
        counters_0.count((bit<32>)hdr.ethernet.kind & 32w7);
        counters_0.count((bit<32>)hdr.ethernet.kind >> 1);
    }
    @hidden action pnacodegencounter23() {
        counters_0.count(32w0);
        counters_0.count(32w7);
        counters_0.count((bit<32>)(hdr.ethernet.kind & 16w7));
    }
    @hidden action pnacodegencounter30() {
        send_to_port(32w1);
    }
    @hidden table tbl_pnacodegencounter22 {
        actions = {
            pnacodegencounter22();
        }
        const default_action = pnacodegencounter22();
    }
    @hidden table tbl_pnacodegencounter23 {
        actions = {
            pnacodegencounter23();
        }
        const default_action = pnacodegencounter23();
    }
    @hidden table tbl_pnacodegencounter27 {
        actions = {
            pnacodegencounter27();
        }
        const default_action = pnacodegencounter27();
    }
    @hidden table tbl_pnacodegencounter30 {
        actions = {
            pnacodegencounter30();
        }
        const default_action = pnacodegencounter30();
    }
    apply {
        if (istd.parser_error != error.NoError) {
            tbl_pnacodegencounter22.apply();
        } else {
            tbl_pnacodegencounter23.apply();
            if (hdr.ethernet.kind != 16w0) {
                tbl_pnacodegencounter27.apply();
            }
            tbl_pnacodegencounter30.apply();
        }
    }
}

control Deparser(packet_out pkt, inout headers_t hdr, in metadata_t meta, in pna_main_output_metadata_t ostd) {
    @hidden action pnacodegencounter35() {
        pkt.emit<ethernet_t>(hdr.ethernet);
    }
    @hidden table tbl_pnacodegencounter35 {
        actions = {
            pnacodegencounter35();
        }
        const default_action = pnacodegencounter35();
    }
    apply {
        tbl_pnacodegencounter35.apply();
    }
}

PNA_NIC<headers_t, metadata_t>(Parser(), Ingress(), Deparser()) main;
