/* -*- P4_16 -*- */

#include <core.p4>
#include <tc/pna.p4>

#define L3_TABLE_SIZE 2048

/*
 * CONST VALUES FOR TYPES
 */
const bit<8> IP_PROTO_TCP = 0x06;
const bit<16> ETHERTYPE_IPV4 = 0x0800;

/*
 * Standard ethernet header
 */
header ethernet_t {
    @tc_type("macaddr") bit<48> dstAddr;
    @tc_type("macaddr") bit<48> srcAddr;
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
    @tc_type("ipv4") bit<32> srcAddr;
    @tc_type("ipv4") bit<32> dstAddr;
}

struct my_ingress_headers_t {
    ethernet_t   ethernet;
    ipv4_t       ipv4;
}

    /******  G L O B A L   I N G R E S S   M E T A D A T A  *********/

struct my_ingress_metadata_t {
    @tc_type("dev") PortId_t ingress_port;
    bool send_digest;
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
        transition select(hdr.ipv4.protocol) {
            IP_PROTO_TCP : accept;
            default: reject;
        }
    }
}

struct mac_learn_digest_t {
    @tc_type("macaddr") bit<48> srcAddr;
    @tc_type("dev") PortId_t ingress_port;
};

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
        meta.ingress_port = istd.input_port;
        meta.send_digest = true;
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
        meta.send_digest = false;

        if (hdr.ipv4.isValid()) {
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
    Digest<bit<32>>() digest_inst;

    apply {
        if (meta.send_digest) {
            digest_inst.pack(16);
        }

        pkt.emit(hdr.ethernet);
        pkt.emit(hdr.ipv4);
    }
}

/************ F I N A L   P A C K A G E ******************************/

PNA_NIC(
    Ingress_Parser(),
    ingress(),
    Ingress_Deparser()
) main;