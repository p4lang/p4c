#include <core.p4>
#include <v1model.p4>

header ethernet_t {
    bit<48> dst_addr;
    bit<48> src_addr;
    bit<16> eth_type;
}

struct nested_struct {
    ethernet_t eth_hdr;
}

struct Headers {
    ethernet_t eth_hdr;
}

struct Meta {
}

parser p(packet_in pkt, out Headers hdr, inout Meta m, inout standard_metadata_t sm) {
    state start {
        pkt.extract<ethernet_t>(hdr.eth_hdr);
        transition accept;
    }
}

control ingress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    apply {
        {
            bool hasReturned = false;
            bit<16> retval;
            nested_struct tmp_struct_0;
            tmp_struct_0.eth_hdr.setValid();
            tmp_struct_0 = (nested_struct){eth_hdr = (ethernet_t){dst_addr = 48w0,src_addr = 48w0,eth_type = 16w0}};
            hasReturned = true;
            retval = tmp_struct_0.eth_hdr.eth_type;
            h.eth_hdr.eth_type = retval;
        }
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
        b.emit<Headers>(h);
    }
}

V1Switch<Headers, Meta>(p(), vrfy(), ingress(), egress(), update(), deparser()) main;

