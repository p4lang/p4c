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
    @hidden action pnacodegenvlandeparser20() {
        send_to_port(32w1);
    }
    @hidden table tbl_pnacodegenvlandeparser20 {
        actions = {
            pnacodegenvlandeparser20();
        }
        const default_action = pnacodegenvlandeparser20();
    }
    apply {
        tbl_pnacodegenvlandeparser20.apply();
    }
}

control Deparser(packet_out pkt, inout headers_t hdr, in metadata_t meta, in pna_main_output_metadata_t ostd) {
    @hidden action pnacodegenvlandeparser25() {
        skb_vlan_pop();
        pkt.emit<ethernet_t>(hdr.ethernet);
    }
    @hidden table tbl_pnacodegenvlandeparser25 {
        actions = {
            pnacodegenvlandeparser25();
        }
        const default_action = pnacodegenvlandeparser25();
    }
    apply {
        tbl_pnacodegenvlandeparser25.apply();
    }
}

PNA_NIC<headers_t, metadata_t>(Parser(), Ingress(), Deparser()) main;
