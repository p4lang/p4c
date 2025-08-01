/* -*- P4_16 -*- */

#include <core.p4>
#include <tc/pna.p4>

struct my_ingress_metadata_t {
}

struct empty_metadata_t {
}

/*
 * Standard ethernet header
 */
header ethernet_t {
    @tc_type("macaddr") bit<48> dstAddr;
    @tc_type("macaddr") bit<48> srcAddr;
    bit<16> etherType;
}

header arp_t {
    bit<16> htype;
    bit<16> ptype;
    bit<8>  hlen;
    bit<8>  plen;
    bit<16> oper;
}

header arp_ipv4_t {
    @tc_type("macaddr") bit<48> sha;
    @tc_type("ipv4") bit<32> spa;
    @tc_type("macaddr") bit<48> tha;
    @tc_type("ipv4") bit<32> tpa;
}

struct my_ingress_headers_t {
    ethernet_t   ethernet;
    arp_t        arp;
    arp_ipv4_t   arp_ipv4;
}

const bit<16> ETHERTYPE_ARP = 0x0806;
const bit<16> ARP_REQ = 1;
const bit<16> ARP_REPLY = 2;

/***********************  P A R S E R  **************************/
parser Ingress_Parser(
        packet_in pkt,
        out   my_ingress_headers_t  hdr,
        inout my_ingress_metadata_t meta,
        in    pna_main_parser_input_metadata_t istd)
{
    state start {
        transition parse_ethernet;
    }

    state parse_ethernet {
        pkt.extract(hdr.ethernet);
        transition select(hdr.ethernet.etherType) {
	    ETHERTYPE_ARP: parse_arp;
            default: accept;
        }
    }

    state parse_arp {
	pkt.extract(hdr.arp);
	pkt.extract(hdr.arp_ipv4);
	transition select(hdr.arp.oper) {
	    ARP_REQ: accept;
            default: reject;
	}
    }
}

#define ARP_TABLE_SIZE 1024

/***************** M A T C H - A C T I O N  *********************/

control ingress(
    inout my_ingress_headers_t  hdr,
    inout my_ingress_metadata_t meta,
    in    pna_main_input_metadata_t  istd,
    inout pna_main_output_metadata_t ostd
)
{
   /* Generate a grat ARP */
   action arp_reply(@tc_type("macaddr") bit<48> rmac) {
	hdr.arp.oper = ARP_REPLY;
	hdr.arp_ipv4.tha = hdr.arp_ipv4.sha;
	hdr.arp_ipv4.sha = rmac;
	hdr.arp_ipv4.spa = hdr.arp_ipv4.tpa;
	hdr.ethernet.dstAddr = hdr.ethernet.srcAddr;
	hdr.ethernet.srcAddr = rmac;
        send_to_port(istd.input_port);
   }

   action drop() {
        drop_packet();
   }

    table arp_table {
        key = {
            hdr.arp_ipv4.tpa : exact @tc_type("ipv4") @name("IPaddr");
        }
        actions = {
            arp_reply;
            drop;
        }
        size = ARP_TABLE_SIZE;
        const default_action = drop;
    }

    apply {
        arp_table.apply();
    }
}

    /*********************  D E P A R S E R  ************************/

control Ingress_Deparser(
    packet_out pkt,
    inout    my_ingress_headers_t hdr,
    in    my_ingress_metadata_t meta,
    in    pna_main_output_metadata_t ostd)
{
    apply {
        pkt.emit(hdr.ethernet);
	pkt.emit(hdr.arp);
	pkt.emit(hdr.arp_ipv4);
    }
}

/************ F I N A L   P A C K A G E ******************************/

PNA_NIC(
    Ingress_Parser(),
    ingress(),
    Ingress_Deparser()
) main;
