#include <core.p4>
#include <v1model.p4>

typedef bit<48>  EthernetAddress;

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

parser SnvsParser(packet_in packet,
		  out headers hdr,
		  inout metadata meta,
		  inout standard_metadata_t standard_metadata) {
    state start {
        packet.extract(hdr.eth);
	transition accept;
    }
}

control SnvsVerifyChecksum(inout headers hdr, inout metadata meta) {
    apply {}
}

typedef bit<(48 + 12 + 9)> Mac_entry;
const bit<32> N_MAC_ENTRIES = 4096;
typedef register<Mac_entry> Mac_table;

control SnvsIngress(inout headers hdr,
		    inout metadata meta,
		    inout standard_metadata_t standard_metadata) {
    Mac_table(N_MAC_ENTRIES) mac_table;

    apply {
	Mac_entry b0;
	mac_table.read(b0, 0);
    }
}

control SnvsEgress(inout headers hdr,
		   inout metadata meta,
		   inout standard_metadata_t standard_metadata) {
    apply {}
}

control SnvsComputeChecksum(inout headers hdr, inout metadata meta) {
    apply {}
}

control SnvsDeparser(packet_out packet, in headers hdr) {
    apply {
        packet.emit(hdr.eth);
    }
}

V1Switch (
    SnvsParser(),
    SnvsVerifyChecksum(),
    SnvsIngress(),
    SnvsEgress(),
    SnvsComputeChecksum(),
    SnvsDeparser()
) main;