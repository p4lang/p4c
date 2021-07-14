/* -*- P4_16 -*- */
#include <core.p4>
#include <psa.p4>

/*************************************************************************
 ************* C O N S T A N T S    A N D   T Y P E S  *******************
*************************************************************************/
#define ETHERTYPE_IPV4  0x0800

/************ H E A D E R S ******************************/
struct EMPTY { };

header ethernet_t {
    bit<48> dst_addr;
    bit<48> src_addr;
    bit<16> ether_type;
}

header ipv4_t {
    bit<8>       version_ihl;
    bit<8>       diffserv;
    bit<16>      total_len;
    bit<16>      identification;
    bit<16>      flags_frag_offset;
    bit<8>       ttl;
    bit<8>       protocol;
    bit<16>      hdr_checksum;
    bit<32>      src_addr;
    bit<32>      dst_addr;
}

struct headers_t {
    ethernet_t  ethernet;
    ipv4_t      ipv4;
}

struct user_meta_data_t {
    bit<32> dst_ip_addr;
    bit<32> vrf_id;
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
        transition select(hdr.ethernet.ether_type) {
            ETHERTYPE_IPV4 :  parse_ipv4;
            default :  accept;
        }        
    }

     state parse_ipv4 {
        pkt.extract(hdr.ipv4);
        m.vrf_id = hdr.ipv4.dst_addr;
        m.dst_ip_addr = hdr.ipv4.dst_addr;        
        transition accept;
    }    
}

control MyIngressControl(
    inout headers_t hdr,
    inout user_meta_data_t m,
    in psa_ingress_input_metadata_t c,
    inout psa_ingress_output_metadata_t d) {

    InternetChecksum() csum;
    ActionSelector(PSA_HashAlgorithm_t.CRC32, 65536, 6) as;

    action drop() {
        d.drop = true;
    }

    action nexthop_action(bit<48> dst_addr, bit<48> src_addr, bit<16> ether_type, PortId_t egress_port) { 
        hdr.ethernet.dst_addr = dst_addr;
        hdr.ethernet.src_addr = src_addr;
        hdr.ethernet.ether_type = ether_type;
        hdr.ethernet.setValid();
        
        if (hdr.ipv4.isValid()) {
            csum.clear();
            csum.add(hdr.ipv4.hdr_checksum);
            csum.subtract(hdr.ipv4.ttl);
            hdr.ipv4.ttl = hdr.ipv4.ttl - 1;
            csum.add(hdr.ipv4.ttl);
            hdr.ipv4.hdr_checksum = csum.get();
         }

        d.egress_port = egress_port;
    }

    table routing_table {
        key = {
            m.vrf_id : exact;
            m.dst_ip_addr : lpm;
            
            hdr.ipv4.protocol : selector;
            hdr.ipv4.src_addr : selector;
            hdr.ipv4.dst_addr : selector;
        }
    
        actions = { 
            NoAction;
            drop;
            nexthop_action;           
        }        
        psa_implementation = as;
        size=1000000;      
    }

    apply {
        routing_table.apply();        
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
        pkt.emit(hdr.ipv4);
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
    apply { }
}

control MyEgressDeparser(
    packet_out pkt,
    out EMPTY a,
    out EMPTY b,
    inout EMPTY c,
    in EMPTY d,
    in psa_egress_output_metadata_t e,
    in psa_egress_deparser_input_metadata_t f) {
    apply { }
}

/************ F I N A L   P A C K A G E ******************************/
IngressPipeline(MyIngressParser(), MyIngressControl(), MyIngressDeparser()) ip;
EgressPipeline(MyEgressParser(), MyEgressControl(), MyEgressDeparser()) ep;

PSA_Switch(ip, PacketReplicationEngine(), ep, BufferingQueueingEngine()) main;