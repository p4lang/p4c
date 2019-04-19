/* -*- P4_16 -*- */
#include <core.p4>
#include <v1model.p4>

typedef bit<9>  egressSpec_t;
typedef bit<48> macAddr_t;
typedef bit<32> ip4Addr_t;

header ethernet_t {
     macAddr_t dstAddr;
     macAddr_t srcAddr;
     bit<16>   etherType;
}

header ipv4_t {
     bit<4>    version;
     bit<4>    ihl;
     bit<8>    diffserv;
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

struct metadata {
     bool test_bool;
}

struct headers {
    ethernet_t   ethernet;
    ipv4_t       ipv4;
}

parser ParserImpl(packet_in packet,
                   out headers hdr,
                   inout metadata meta,
                   inout standard_metadata_t standard_metadata) {


     state start {
         packet.extract(hdr.ethernet);
         transition select(hdr.ethernet.etherType) {
             0x800: parse_ipv4;
             default: reject;
         }
     }

     state parse_ipv4 {
         packet.extract(hdr.ipv4);
         transition accept;
     }

}

control verifyChecksum(inout headers hdr, inout metadata meta) {
     apply {  }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    action drop() {
        mark_to_drop(standard_metadata);
    }

    action ipv4_forward(macAddr_t dstAddr, egressSpec_t port) {
        meta.test_bool = true;
     }

    table ipv4_lpm {
        key = {
            hdr.ipv4.dstAddr: lpm;
        }
        actions = {
            NoAction;
            ipv4_forward;  drop;
        }
        size = 1024;
         default_action = NoAction();
    }

    apply {
        meta.test_bool = false;

        if (hdr.ipv4.isValid()) {
            ipv4_lpm.apply();
        }

        if (!meta.test_bool)
            drop();
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
     apply {  }
}

control computeChecksum(
     inout headers  hdr,
     inout metadata meta)
{
     apply {  }
}


control DeparserImpl(packet_out packet, in headers hdr) {
     apply {
        packet.emit(hdr);
     }
}

V1Switch(
ParserImpl(),
verifyChecksum(),
ingress(),
egress(),
computeChecksum(),
DeparserImpl()
) main;
