/* -*- P4_16 -*- */

#include <core.p4>
#include <tc/pna.p4>

/* -*- P4_16 -*- */

struct my_ingress_metadata_t {
}

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

header csum_t {
   bit<16> crc16;
   bit<32> crc32;
};

struct my_ingress_headers_t {
    ethernet_t   ethernet;
    ipv4_t       ipv4;
    csum_t       csum;
}

/***********************  P A R S E R  **************************/
parser Ingress_Parser(
        packet_in pkt,
        out   my_ingress_headers_t  hdr,
        inout my_ingress_metadata_t meta,
        in    pna_main_parser_input_metadata_t istd)
{
    const bit<16> ETHERTYPE_IPV4 = 0x0800;
    const bit<8> PROTO_CSUM = 0x92;

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
            PROTO_CSUM: parse_csum;
            default: reject;
        }
    }

    state parse_csum {
         pkt.extract(hdr.csum);
         transition accept;
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
    apply {
    }
}

    /*********************  D E P A R S E R  ************************/

control Ingress_Deparser(
    packet_out pkt,
    inout    my_ingress_headers_t hdr,
    in    my_ingress_metadata_t meta,
    in    pna_main_output_metadata_t ostd)
{
   Checksum<bit<32>>(PNA_HashAlgorithm_t.CRC32) c32;
   Checksum<bit<16>>(PNA_HashAlgorithm_t.CRC16) c16;

    apply {
        c32.update(hdr.ipv4.srcAddr);
        c32.update(hdr.ipv4.dstAddr);
        hdr.csum.crc32 = c32.get();
        c32.clear();

        c16.update(hdr.ipv4.protocol);
        c16.update(hdr.ipv4.ttl);
        hdr.csum.crc16 = c16.get();
        c16.clear();

        pkt.emit(hdr.ethernet);
        pkt.emit(hdr.ipv4);
        pkt.emit(hdr.csum);
    }
}

/************ F I N A L   P A C K A G E ******************************/

PNA_NIC(
    Ingress_Parser(),
    ingress(),
    Ingress_Deparser()
) main;
