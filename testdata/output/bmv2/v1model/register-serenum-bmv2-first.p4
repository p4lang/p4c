#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

enum bit<16> EthTypes {
    IPv4 = 16w0x800,
    ARP = 16w0x806,
    RARP = 16w0x8035,
    EtherTalk = 16w0x809b,
    VLAN = 16w0x8100,
    IPX = 16w0x8137,
    IPv6 = 16w0x86dd
}

header ethernet_t {
    bit<48>  dst_addr;
    bit<48>  src_addr;
    EthTypes eth_type;
}

struct Headers {
    ethernet_t eth_hdr;
}

struct Meta {
}

parser p(packet_in pkt, out Headers hdr, inout Meta m, inout standard_metadata_t sm) {
    ethernet_t e;
    state start {
        pkt.extract<ethernet_t>(e);
        transition select(e.eth_type) {
            EthTypes.IPv4: accept;
            EthTypes.ARP: accept;
            default: reject;
        }
    }
}

control ingress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    register<EthTypes>(32w1) reg;
    apply {
        reg.write(32w0, h.eth_hdr.eth_type);
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

control deparser(packet_out pkt, in Headers h) {
    apply {
        pkt.emit<Headers>(h);
    }
}

V1Switch<Headers, Meta>(p(), vrfy(), ingress(), egress(), update(), deparser()) main;
