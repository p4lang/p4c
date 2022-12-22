#include <core.p4>
#include <v1model.p4>

header ethernet_t {
    bit<48> dst_addr;
    bit<48> src_addr;
    bit<16> eth_type;
    bit<32> index;
    bit<4> index1;
    bit<4> index2;
    bit<4> index3;
    bit<4> index4;
    bit<4> index5;
    bit<4> index6;
}

struct headers {
    ethernet_t eth_hdr;
}

struct Meta {}

parser p(packet_in pkt, out headers hdr, inout Meta m, inout standard_metadata_t sm) {
    state start {
        transition parse_hdrs;
    }
    state parse_hdrs {
        pkt.extract(hdr.eth_hdr);
        transition accept;
    }
}

control ingress(inout headers h, inout Meta m, inout standard_metadata_t sm) {
    bit<32>  a = 0;
    bit<32>  b = 0;
    bit<32>  c = 0;
    bit<16>  tmp;
    bit<4> res;
    apply {
        h.eth_hdr.index2 = 4w4;
        h.eth_hdr.index3 = 4w4;
        h.eth_hdr.index5 = 4w4;
        h.eth_hdr.index1 = (h.eth_hdr.index2 ++ h.eth_hdr.index3 ++ h.eth_hdr.index4 ++ h.eth_hdr.index5 ++ h.eth_hdr.index6)[11:8];
        if (h.eth_hdr.isValid())
            a = 1;
        else
            a = 0;
        h.eth_hdr.index = 2;
        h.eth_hdr.eth_type = 1;
        h.eth_hdr.setInvalid();
        if (h.eth_hdr.isValid())
            b = 1;
        else
            b = 0;
        h.eth_hdr.setValid();
        if (h.eth_hdr.isValid() && h.eth_hdr.index == 0)
            c = 1;
        else
            c = 0;
    }
}

control vrfy(inout headers h, inout Meta m) {
    apply {}
 }

control update(inout headers hdr, inout Meta m) { apply {    
}}

control egress(inout headers h, inout Meta m, inout standard_metadata_t sm) { apply {} }

control deparser(packet_out pkt, in headers h) {
    apply {
        pkt.emit(h.eth_hdr);
    }
}
V1Switch(p(), vrfy(), ingress(), egress(), update(), deparser()) main;

