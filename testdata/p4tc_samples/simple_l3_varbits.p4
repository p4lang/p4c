/* -*- P4_16 -*- */

#include <core.p4>
#include <tc/pna.p4>

struct my_ingress_metadata_t {
}

struct empty_metadata_t {
}

/*
 * CONST VALUES FOR TYPES
 */
const bit<8> IP_PROTO_TCP = 0x06;
const bit<16> ETHERTYPE_IPV4 = 0x0800;

/*
 * Standard ethernet header
 */
header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
}

header ipv4_t {
    bit<4>  version;
    bit<4>  ihl;
    bit<8>  diffserv;
    bit<16> totalLen;
    bit<16> identification;
    bit<3>  flags;
    bit<13> fragOffset;
    bit<8>  ttl;
    bit<8>  protocol;
    bit<16> hdrChecksum;
    bit<32> srcAddr;
    bit<32> dstAddr;
}

header v4_options_t {
    varbit<320> opts;
}

struct my_ingress_headers_t {
    ethernet_t   ethernet;
    ipv4_t       ipv4;
    v4_options_t opts;
}

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
            ETHERTYPE_IPV4: parse_ipv4;
            default: reject;
        }
    }
    state parse_ipv4 {
        pkt.extract(hdr.ipv4);
	transition select(hdr.ipv4.ihl) {
	    4w0: reject;
	    4w1: reject;
	    4w2: reject;
	    4w3: reject;
	    4w4: reject;
	    5: parse_proto;
	    default: parse_v4_opts;
	}
    }
    state parse_v4_opts {
	// XXX The cast to bit<32> is because pkt.extract demands
	//  bit<32> for its second arg
	pkt.extract(hdr.opts,(bit<32>)(32*(bit<9>)(hdr.ipv4.ihl-5)));
	transition parse_proto;
    }
    state parse_proto {
        transition select(hdr.ipv4.protocol) {
            IP_PROTO_TCP : accept;
            default: reject;
        }
    }
}

#define L3_TABLE_SIZE 2048

/***************** M A T C H - A C T I O N  *********************/

control ingress(
    inout my_ingress_headers_t  hdr,
    inout my_ingress_metadata_t meta,
    in    pna_main_input_metadata_t  istd,
    inout pna_main_output_metadata_t ostd
)
{
   action send_nh(@tc_type("dev") PortId_t port, @tc_type("macaddr") bit<48> srcMac, @tc_type("macaddr") bit<48> dstMac) {
        hdr.ethernet.srcAddr = srcMac;
        hdr.ethernet.dstAddr = dstMac;
        hdr.ipv4.ttl = 31;
        send_to_port(port);
   }

   action drop() {
        drop_packet();
   }

    table nh_table {
        key = {
            hdr.ipv4.dstAddr : exact @tc_type("ipv4") @name("dstAddr");
        }
        actions = {
            send_nh;
            drop;
        }
        size = L3_TABLE_SIZE;
        const default_action = drop;
    }

    apply {
        if (hdr.ipv4.isValid() && hdr.ipv4.protocol == IP_PROTO_TCP) {
            nh_table.apply();
        }
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
        pkt.emit(hdr.ipv4);
        pkt.emit(hdr.opts);
    }
}

/************ F I N A L   P A C K A G E ******************************/

PNA_NIC(
    Ingress_Parser(),
    ingress(),
    Ingress_Deparser()
) main;
