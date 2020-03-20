#include <core.p4>
#include <v1model.p4>

header ethernet_t {
    bit<48> dst_addr;
    bit<48> src_addr;
    bit<16> eth_type;
}

struct Headers {
    ethernet_t eth_hdr;
}

struct Meta {
}

bit<16> function_with_side_effect(inout bit<16> eth_type) {
    eth_type = 0x0806;
    return 16w2;
}
parser p(packet_in pkt, out Headers hdr, inout Meta m, inout standard_metadata_t sm) {
    state start {
        pkt.extract(hdr.eth_hdr);
        transition accept;
    }
}

control ingress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    apply {
        bit<16> dummy_var;
        dummy_var = 16w0 & function_with_side_effect(h.eth_hdr.eth_type);
        dummy_var = 16w0 * function_with_side_effect(h.eth_hdr.eth_type);
        dummy_var = 16w0 >> function_with_side_effect(h.eth_hdr.eth_type);
    }
}

control vrfy(inout Headers h, inout Meta m) { apply {} }

control update(inout Headers h, inout Meta m) { apply {} }

control egress(inout Headers h, inout Meta m, inout standard_metadata_t sm) { apply {} }

control deparser(packet_out b, in Headers h) { apply {b.emit(h);} }

V1Switch(p(), vrfy(), ingress(), egress(), update(), deparser()) main;
