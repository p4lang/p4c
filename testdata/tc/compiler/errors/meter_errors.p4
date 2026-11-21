/* -*- P4_16 -*- */

#include <core.p4>
#include <tc/pna.p4>

#define METER_TABLE_SIZE 2048

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
    bit<32> srcAddr;
    bit<32> dstAddr;
}

struct my_ingress_headers_t {
    ethernet_t ethernet;
    ipv4_t     ipv4;
}

/******  G L O B A L   I N G R E S S   M E T A D A T A  *********/

struct my_ingress_metadata_t {
}

struct empty_metadata_t {
}

/***********************  P A R S E R  **************************/

parser Ingress_Parser(
        packet_in pkt,
        out   my_ingress_headers_t  hdr,
        inout my_ingress_metadata_t meta,
        in    pna_main_parser_input_metadata_t istd)
{
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
    Meter<bit<32>>(2048, PNA_MeterType_t.PACKETS) global_meter;

    action meter_exec() {
        PNA_MeterColor_t color;

        color = global_meter.execute(3, PNA_MeterColor_t.RED);
        if (color == PNA_MeterColor_t.RED) {
                drop_packet();
        }
    }
    action drop() {
        drop_packet();
    }
    @tc_acl("CRUDPS:RX") table nh_table {
        key = {
            hdr.ipv4.srcAddr : exact @tc_type ("ipv4");
        }
        actions = {
            meter_exec;
            drop;
        }
        size = METER_TABLE_SIZE;
        const default_action = drop;
        pna_direct_meter = global_meter;
    }

    apply {
        nh_table.apply();
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