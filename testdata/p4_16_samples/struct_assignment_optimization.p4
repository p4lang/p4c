#include <core.p4>
#include <v1model.p4>

typedef bit<48>  EthernetAddress;

header ethernet_t {
    bit<48> dst_addr;
    bit<48> src_addr;
    bit<16> eth_type;
}

struct simple_struct {
    bit<128> a;
}

struct Headers {
    ethernet_t eth_hdr;
}

struct Metadata {}


parser p(packet_in pkt,
               out Headers hdr, inout Metadata meta,
               inout standard_metadata_t stdmeta) {
    state start {
        pkt.extract(hdr.eth_hdr);
        transition accept;
    }
}


control ingress(inout Headers hdr, inout Metadata meta,
                 inout standard_metadata_t stdmeta) {
    simple_struct test = { 128w0 };
    action pointless_action() {
        test = test;
    }

    apply {
        pointless_action();
        if ((test.a != 1) && (test.a <= 1)) {
            hdr.eth_hdr.eth_type = 16w1;
        }
    }
}


control deparser(packet_out packet, in Headers hdr) {
    apply {
        packet.emit(hdr);
    }
}

control egress(inout Headers hdr, inout Metadata meta, inout standard_metadata_t stdmeta) {
    apply {}
}

control vrfy(inout Headers hdr, inout Metadata meta) {
    apply {}
}

control update(inout Headers hdr, inout Metadata meta) {
    apply {}
}

V1Switch(p(), vrfy(), ingress(), egress(), update(), deparser()) main;
