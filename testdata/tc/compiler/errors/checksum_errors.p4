/* -*- P4_16 -*- */

#include <core.p4>
#include <tc/pna.p4>

#define PORT_TABLE_SIZE 262144

/*
 * Standard ethernet header
 */
header ethernet_t {
    @tc_type ("macaddr") bit<48> dstAddr;
    @tc_type ("macaddr") bit<48> srcAddr;
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
    @tc_type ("ipv4") bit<32> srcAddr;
    @tc_type ("ipv4") bit<32> dstAddr;
}

struct my_ingress_headers_t {
    ethernet_t ethernet;
    ipv4_t     ipv4;
}

/******  G L O B A L   I N G R E S S   M E T A D A T A  *********/

struct my_ingress_metadata_t {
    bit<16> checksum;
    bit<16> state;
}

struct empty_metadata_t {
}

error {
    BadIPv4HeaderChecksum
}

/***********************  P A R S E R  **************************/

parser Ingress_Parser(
        packet_in pkt,
        out   my_ingress_headers_t  hdr,
        inout my_ingress_metadata_t meta,
        in    pna_main_parser_input_metadata_t istd)
{
    InternetChecksum() ck;
    const bit<16> ETHERTYPE_IPV4 = 0x0800;

    state start {
        transition parse_ethernet;
    }
    state parse_ethernet {
        pkt.extract(hdr.ethernet);
        transition select(hdr.ethernet.etherType) {
            ETHERTYPE_IPV4 : parse_ipv4;
            default        : reject;
        }
    }
    state parse_ipv4 {
        pkt.extract(hdr.ipv4);
        ck.clear();
        ck.add({
            /* 16-bit word 0 */     hdr.ipv4.version, hdr.ipv4.ihl, hdr.ipv4.diffserv,
            /* 16-bit word 1 */     hdr.ipv4.totalLen,
            /* 16-bit word 2 */     hdr.ipv4.identification,
            /* 16-bit word 3 */     hdr.ipv4.flags, hdr.ipv4.fragOffset,
            /* 16-bit word 4 */     hdr.ipv4.ttl, hdr.ipv4.protocol,
            /* 16-bit word 5 skip hdr.ipv4.hdrChecksum, */
            /* 16-bit words 6-7 */  hdr.ipv4.srcAddr,
            /* 16-bit words 8-9 */  hdr.ipv4.dstAddr
            });
        verify(hdr.ipv4.hdrChecksum == ck.get(), error.BadIPv4HeaderChecksum);
        hdr.ipv4.hdrChecksum = ck.get_state();
        transition accept;
    }
}

/***************** M A T C H - A C T I O N  *********************/

control ingress(
    inout my_ingress_headers_t  hdr,
    inout my_ingress_metadata_t meta,
    in    pna_main_input_metadata_t  istd,
    inout pna_main_output_metadata_t ostd
)
{
    InternetChecksum() ck;
    apply {
        ck.clear();
        ck.add({
            /* 16-bit word 0 */     hdr.ipv4.version, hdr.ipv4.ihl, hdr.ipv4.diffserv,
            /* 16-bit word 1 */     hdr.ipv4.totalLen,
            /* 16-bit word 2 */     hdr.ipv4.identification,
            /* 16-bit word 3 */     hdr.ipv4.flags, hdr.ipv4.fragOffset,
            /* 16-bit word 4 */     hdr.ipv4.ttl, hdr.ipv4.protocol,
            /* 16-bit word 5 skip hdr.ipv4.hdrChecksum, */
            /* 16-bit words 6-7 */  hdr.ipv4.srcAddr,
            /* 16-bit words 8-9 */  hdr.ipv4.dstAddr
            });
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
    }
}

/************ F I N A L   P A C K A G E ******************************/

PNA_NIC(
    Ingress_Parser(),
    ingress(),
    Ingress_Deparser()
) main;
