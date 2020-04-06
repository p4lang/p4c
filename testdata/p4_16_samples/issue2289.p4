#include <core.p4>
#include <v1model.p4>

header ethernet_t {
    bit<48> dst_addr;
    bit<48> src_addr;
    bit<16> eth_type;
}

header simple_struct {
    bit<32> a;
}

struct nested_struct {
    simple_struct  s;
}

struct Headers {
    ethernet_t eth_hdr;
}

struct Meta {
}

bit<16> function_1() {
    nested_struct tmp_struct = {{1}};
    tmp_struct.s.a = 1;
    return (bit<16>)(1);
}
bit<16> function_2(in bit<16> val) {
    return function_1();
}
parser p(packet_in pkt, out Headers hdr, inout Meta m, inout standard_metadata_t sm) {
    state start {
        transition parse_hdrs;
    }
    state parse_hdrs {
        pkt.extract(hdr.eth_hdr);
        transition accept;
    }
}

control ingress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    action simple_action(out bit<16> byaA) {
        h.eth_hdr.eth_type = function_2(function_1());
    }
    apply {
        function_1();
        simple_action(h.eth_hdr.eth_type);


    }
}

control vrfy(inout Headers h, inout Meta m) { apply {} }

control update(inout Headers h, inout Meta m) { apply {} }

control egress(inout Headers h, inout Meta m, inout standard_metadata_t sm) { apply {} }

control deparser(packet_out b, in Headers h) { apply {b.emit(h);} }

V1Switch(p(), vrfy(), ingress(), egress(), update(), deparser()) main;
