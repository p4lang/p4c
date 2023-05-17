#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

typedef bit<48> EthernetAddress;
header Ethernet_h {
    EthernetAddress dst;
    EthernetAddress src;
    bit<16>         type;
}

struct metadata {
}

struct headers {
    Ethernet_h eth;
}

parser SnvsParser(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    state start {
        packet.extract<Ethernet_h>(hdr.eth);
        transition accept;
    }
}

control SnvsVerifyChecksum(inout headers hdr, inout metadata meta) {
    apply {
    }
}

typedef bit<69> Mac_entry;
const bit<32> N_MAC_ENTRIES = 32w4096;
typedef register<Mac_entry> Mac_table;
control SnvsIngress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    Mac_table(32w4096) mac_table;
    apply {
        Mac_entry b0;
        mac_table.read(b0, 32w0);
    }
}

control SnvsEgress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control SnvsComputeChecksum(inout headers hdr, inout metadata meta) {
    apply {
    }
}

control SnvsDeparser(packet_out packet, in headers hdr) {
    apply {
        packet.emit<Ethernet_h>(hdr.eth);
    }
}

V1Switch<headers, metadata>(SnvsParser(), SnvsVerifyChecksum(), SnvsIngress(), SnvsEgress(), SnvsComputeChecksum(), SnvsDeparser()) main;
