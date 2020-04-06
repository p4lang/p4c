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
    simple_struct s;
}

struct Headers {
    ethernet_t eth_hdr;
}

struct Meta {
}

bit<16> function_1() {
    simple_struct tmp_struct_0_s;
    tmp_struct_0_s.setValid();
    return 16w1;
}
parser p(packet_in pkt, out Headers hdr, inout Meta m, inout standard_metadata_t sm) {
    state start {
        pkt.extract<ethernet_t>(hdr.eth_hdr);
        transition accept;
    }
}

control ingress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    simple_struct tmp_struct_1_s;
    simple_struct tmp_struct_2_s;
    bit<16> byaA;
    @name("ingress.simple_action") action simple_action() {
        tmp_struct_1_s.setValid();
        tmp_struct_2_s.setValid();
        h.eth_hdr.eth_type = byaA;
    }
    @hidden action issue2289l48() {
        function_1();
    }
    @hidden table tbl_issue2289l48 {
        actions = {
            issue2289l48();
        }
        const default_action = issue2289l48();
    }
    @hidden table tbl_simple_action {
        actions = {
            simple_action();
        }
        const default_action = simple_action();
    }
    apply {
        tbl_issue2289l48.apply();
        tbl_simple_action.apply();
    }
}

control vrfy(inout Headers h, inout Meta m) {
    apply {
    }
}

control update(inout Headers h, inout Meta m) {
    apply {
    }
}

control egress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    apply {
    }
}

control deparser(packet_out b, in Headers h) {
    apply {
        b.emit<ethernet_t>(h.eth_hdr);
    }
}

V1Switch<Headers, Meta>(p(), vrfy(), ingress(), egress(), update(), deparser()) main;

