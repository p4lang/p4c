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
    apply {
        if (istd.parser_error != error.NoError) {
            drop_packet();
        } else {
            counters_0.count(32w0);
            counters_0.count(32w7);
            counters_0.count((bit<32>)(hdr.ethernet.kind & 16w7));
            if (hdr.ethernet.kind != 16w0) {
                counters_0.count((bit<32>)hdr.ethernet.kind & 32w7);
                counters_0.count((bit<32>)hdr.ethernet.kind >> 1);
            }
            send_to_port(istd.input_port);
        }
    }
}

control Deparser(packet_out pkt, inout headers_t hdr, in metadata_t meta, in pna_main_output_metadata_t ostd) {
    apply {
        pkt.emit<ethernet_t>(hdr.ethernet);
    }
}

PNA_NIC<headers_t, metadata_t>(Parser(), Ingress(), Deparser()) main;
