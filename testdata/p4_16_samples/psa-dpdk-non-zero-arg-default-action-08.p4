/* -*- P4_16 -*- */
#include <core.p4>
#include <psa.p4>

/************ H E A D E R S ******************************/
struct EMPTY {};

header ethernet_t {
    bit<48> dst_addr;
    bit<48> src_addr;
    bit<16> ether_type;
}

struct headers_t {
    ethernet_t  ethernet;
}

struct user_meta_data_t {
    bit<48> addr;
}

/*************************************************************************
 ****************  I N G R E S S   P R O C E S S I N G   *****************
 *************************************************************************/
parser MyIngressParser(
    packet_in pkt,
    out headers_t hdr,
    inout user_meta_data_t m,
    in psa_ingress_parser_input_metadata_t c,
    in EMPTY d,
    in EMPTY e) {

    state start {
        pkt.extract(hdr.ethernet);
        transition accept;
    }
}

control MyIngressControl(
    inout headers_t hdr,
    inout user_meta_data_t m,
    in psa_ingress_input_metadata_t c,
    inout psa_ingress_output_metadata_t d) {
    action nonDefAct() {
        m.addr = hdr.ethernet.dst_addr;
        hdr.ethernet.dst_addr = hdr.ethernet.src_addr;
        hdr.ethernet.src_addr = m.addr;
    }
	action macswp(inout bit<48> tmp1, bit<32> tmp2) {
		if (tmp1 == 0x1 && tmp2 == 0x2) {
			m.addr = hdr.ethernet.dst_addr;
			hdr.ethernet.dst_addr = hdr.ethernet.src_addr;
			hdr.ethernet.src_addr = m.addr;
		}
	}
    table stub {
        key = {}

        actions = {
            macswp(hdr.ethernet.dst_addr);
			nonDefAct;
        }
        default_action = macswp(hdr.ethernet.dst_addr, 3);
        size=1000000;
    }

    apply {
        d.egress_port = (PortId_t) ((bit <32>) c.ingress_port ^ 1);
        stub.apply();
    }
}

control MyIngressDeparser(
    packet_out pkt,
    out EMPTY a,
    out EMPTY b,
    out EMPTY c,
    inout headers_t hdr,
    in user_meta_data_t e,
    in psa_ingress_output_metadata_t f) {

    apply {
        pkt.emit(hdr.ethernet);
    }
}

/*************************************************************************
 ****************  E G R E S S   P R O C E S S I N G   *******************
 *************************************************************************/
parser MyEgressParser(
    packet_in pkt,
    out EMPTY a,
    inout EMPTY b,
    in psa_egress_parser_input_metadata_t c,
    in EMPTY d,
    in EMPTY e,
    in EMPTY f) {
    state start {
        transition accept;
    }
}

control MyEgressControl(
    inout EMPTY a,
    inout EMPTY b,
    in psa_egress_input_metadata_t c,
    inout psa_egress_output_metadata_t d) {
    apply {}
}
control MyEgressDeparser(
    packet_out pkt,
    out EMPTY a,
    out EMPTY b,
    inout EMPTY c,
    in EMPTY d,
    in psa_egress_output_metadata_t e,
    in psa_egress_deparser_input_metadata_t f) {
    apply {}
}

/************ F I N A L   P A C K A G E ******************************/
IngressPipeline(MyIngressParser(), MyIngressControl(), MyIngressDeparser()) ip;

EgressPipeline(MyEgressParser(), MyEgressControl(), MyEgressDeparser()) ep;

PSA_Switch(ip, PacketReplicationEngine(), ep, BufferingQueueingEngine()) main;
