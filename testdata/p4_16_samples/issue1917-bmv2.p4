/* -*- P4_16 -*- */
#include <core.p4>
#include <v1model.p4>

const bit<16> TYPE_IPV4 = 0x800;
#define RECIRCULATE_TIMES 5

/*************************************************************************
*********************** H E A D E R S  ***********************************
*************************************************************************/

typedef bit<9>  egressSpec_t;
typedef bit<48> macAddr_t;
typedef bit<32> ip4Addr_t;

/* TCP/IP Headers */

header ethernet_t {
    macAddr_t dstAddr;
    macAddr_t srcAddr;
    bit<16>   etherType;
}

header ipv4_t {
    bit<4>    version;
    bit<4>    ihl;
    bit<8>    tos;
    bit<16>   totalLen;
    bit<16>   identification;
    bit<3>    flags;
    bit<13>   fragOffset;
    bit<8>    ttl;
    bit<8>    protocol;
    bit<16>   hdrChecksum;
    ip4Addr_t srcAddr;
    ip4Addr_t dstAddr;
}

struct count_struct_t {
    bit<8> counter;
}

struct metadata {
    count_struct_t count_struct;
}

struct headers {
    ethernet_t   ethernet;
    ipv4_t       ipv4;
}

struct h {
    headers hdr;
}

/*************************************************************************
*********************** P A R S E R  ***********************************
*************************************************************************/

parser MyParser(packet_in packet,
                out h hdr,
                inout metadata meta,
                inout standard_metadata_t standard_metadata) {
    state start {
        packet.extract(hdr.hdr.ethernet);
        transition select(hdr.hdr.ethernet.etherType){
            TYPE_IPV4: ipv4;
            default: accept;
        }
    }
    state ipv4 {
        packet.extract(hdr.hdr.ipv4);
        transition accept;
    }
}

/*************************************************************************
***********************  D E P A R S E R  *******************************
*************************************************************************/

control MyDeparser(packet_out packet, in h hdr) {
    apply {
        packet.emit(hdr.hdr.ethernet);
        packet.emit(hdr.hdr.ipv4);
    }
}

/*************************************************************************
************   C H E C K S U M    V E R I F I C A T I O N   *************
*************************************************************************/

control MyVerifyChecksum(inout h hdr, inout metadata meta) {
    apply {  }
}

/*************************************************************************
**************  I N G R E S S   P R O C E S S I N G   *******************
*************************************************************************/

control MyIngress(inout h hdr,
                  inout metadata meta,
                  inout standard_metadata_t standard_metadata) {
    apply {
        standard_metadata.egress_spec = 2;
        standard_metadata.ingress_port = (bit<9>)standard_metadata.enq_timestamp;
    }
}

/*************************************************************************
****************  E G R E S S   P R O C E S S I N G   *******************
*************************************************************************/

control MyEgress(inout h hdr,
                 inout metadata meta,
                 inout standard_metadata_t standard_metadata) {
    apply {
        standard_metadata.deq_qdepth = 1;
        standard_metadata.egress_spec = 0;
    }
}

/*************************************************************************
*************   C H E C K S U M    C O M P U T A T I O N   **************
*************************************************************************/

control MyComputeChecksum(inout h hdr, inout metadata meta) {
     apply {    }
}


/*************************************************************************
***********************  S W I T C H  *******************************
*************************************************************************/

//switch architecture
V1Switch(
MyParser(),
MyVerifyChecksum(),
MyIngress(),
MyEgress(),
MyComputeChecksum(),
MyDeparser()
) main;
