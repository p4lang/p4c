#include <core.p4>
#include <v1model.p4>

enum bit<16> EthTypes {
    IPv4 = 0x0800,
    ARP = 0x0806,
    RARP = 0x8035,
    EtherTalk = 0x809B,
    VLAN = 0x8100,
    IPX = 0x8137,
    IPv6 = 0x86DD
}

header ethernet_t {
    bit<48> dst_addr;
    bit<48> src_addr;
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
        pkt.extract(e);
        transition select(e.eth_type) {
            EthTypes.IPv4: accept;
            EthTypes.ARP: accept;
            default: reject;
        }
    }
}

control ingress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    register<EthTypes>(1) reg;

    apply {
        reg.write(0, h.eth_hdr.eth_type);
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
        pkt.emit(h);
    }
}


V1Switch(p(), vrfy(), ingress(), egress(), update(), deparser()) main;
