/* -*- P4_16 -*- */

#include <core.p4>
#include <tc/pna.p4>

/* -*- P4_16 -*- */

    /******  G L O B A L   I N G R E S S   M E T A D A T A  *********/

struct my_ingress_metadata_t {
}

struct empty_metadata_t {
}

/* -*- P4_16 -*- */

/*
 * CONST VALUES FOR TYPES
 */
const bit<8> IP_PROTO_UDP = 0x11;
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
    varbit<320> o;
}

header udp_t {
    bit<16> src_port;
    bit<16> dst_port;
    bit<16> len;
    bit<16> csum;
}

struct my_ingress_headers_t {
    ethernet_t   ethernet;
    ipv4_t       ipv4;
    v4_options_t opts;
    udp_t udp;
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
        pkt.extract(hdr.opts,(bit<32>)(32*(bit<9>)(hdr.ipv4.ihl-5)));
        transition parse_proto;
    }

    state parse_proto {
        transition select(hdr.ipv4.protocol) {
            IP_PROTO_UDP: parse_udp;
            default: reject;
        }
    }

    state parse_udp {
        pkt.extract(hdr.udp);
        transition accept;
    }
}

void ip_checksum_recalc(InternetChecksum chk, inout ipv4_t ip, bit<8> old_proto, in bit<16> old_total_len) {
   chk.clear();
   chk.set_state(~ip.hdrChecksum);

   chk.subtract({ old_total_len });
   chk.subtract({ ip.ttl, old_proto });
   chk.add({ ip.ttl, ip.protocol });
   chk.add({ ip.totalLen });

   ip.hdrChecksum = chk.get();
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
    action reflect() {
        bit<48> tmp_mac;

        tmp_mac = hdr.ethernet.dstAddr;
        hdr.ethernet.dstAddr = hdr.ethernet.srcAddr;
        hdr.ethernet.srcAddr = tmp_mac;

        hdr.ipv4.totalLen -= hdr.udp.len;
        hdr.ipv4.protocol = 253;
        hdr.udp.setInvalid();
        send_to_port(istd.input_port);
    }

    action drop() {
        drop_packet();
    }

    table nh_table {
        key = {
            hdr.ipv4.dstAddr : exact @tc_type("ipv4") @name("dstAddr");
        }
        actions = {
            reflect;
            drop;
        }
        size = L3_TABLE_SIZE;
        const default_action = reflect;
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
    InternetChecksum() chk;

    apply {
        pkt.emit(hdr.ethernet);
        ip_checksum_recalc(chk, hdr.ipv4, IP_PROTO_UDP, hdr.ipv4.totalLen + hdr.udp.len);
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
