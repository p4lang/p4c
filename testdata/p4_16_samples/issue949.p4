#define DCE_BUG 1


#include <core.p4>
#include <v1model.p4>

struct routing_metadata_t {
	bit<32> nhop_ipv4;
}

header ethernet_t {
	bit<48>	dstAddr;
	bit<48>	srcAddr;
	bit<16>	etherType;
}

header ipv4_t {
	bit<4>	version;
	bit<4>	ihl;
	bit<8>	diffserv;
	// ...
}

struct metadata {
	routing_metadata_t	routing_metadata;
}

struct headers {
	ethernet_t	ethernet;
	ipv4_t		ipv4;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
	state start {
		packet.extract(hdr.ethernet);
		transition select(hdr.ethernet.etherType) {
			16w0x800: parse_ipv4;
			default: accept;
		}
	}
	state parse_ipv4 {
		packet.extract(hdr.ipv4);
		transition accept;
	}
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
	apply { }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
	action setDest() {
		hdr.ethernet.dstAddr = 48w0x_6A_F3_40_04_26_D3;
	}
	table someTable {
		key = {
			hdr.ethernet.srcAddr : exact;
		}
		actions = {
			setDest;
		}
	}
	apply {
#if DCE_BUG
		bool didHit = someTable.apply().hit;
#else
		someTable.apply();
#endif
	}
}

control DeparserImpl(packet_out packet, in headers hdr) {
	apply {
		packet.emit(hdr.ethernet);
		packet.emit(hdr.ipv4);
	}
}

control verifyChecksum(inout headers hdr, inout metadata meta) {
	apply { }
}

control computeChecksum(inout headers hdr, inout metadata meta) {
	apply { }
}

V1Switch(ParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;
