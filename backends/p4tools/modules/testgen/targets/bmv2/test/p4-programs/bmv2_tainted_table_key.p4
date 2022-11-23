#include <core.p4>
#include <v1model.p4>

header ethernet_t {
    bit<48> dst_addr;
    bit<48> src_addr;
    bit<16> eth_type;
}


struct headers {
    ethernet_t eth_hdr;
}

struct Meta {}

parser p(packet_in pkt, out headers hdr, inout Meta m, inout standard_metadata_t sm) {
    state start {
        pkt.extract(hdr.eth_hdr);
        transition accept;
    }
}

control ingress(inout headers h, inout Meta m, inout standard_metadata_t sm) {
    action set_output_port(bit<9> out_port) {
        hash(sm.egress_spec, HashAlgorithm.crc16, 16w0, {h.eth_hdr.dst_addr, out_port, 16w0xAAAA}, 16w65535);
    }

    table forward_table {
        key = {
            h.eth_hdr.src_addr : exact;
        }
        actions = {
            NoAction;
            set_output_port;
        }

        default_action = NoAction();
    }

    apply {
        h.eth_hdr.src_addr = 48w1;
        forward_table.apply();
    }
}

control vrfy(inout headers h, inout Meta m) { apply {} }

control update(inout headers h, inout Meta m) { apply {} }

control egress(inout headers h, inout Meta m, inout standard_metadata_t sm) { apply {} }

control deparser(packet_out pkt, in headers h) {
    apply {
        pkt.emit(h.eth_hdr);
    }
}
V1Switch(p(), vrfy(), ingress(), egress(), update(), deparser()) main;
