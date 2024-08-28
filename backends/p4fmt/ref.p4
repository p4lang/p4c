/* -*- P4_16 -*- */
#include <core.p4>
#include <v1model.p4>

const bit<16> TYPE_IPV4 = 0x800;

typedef bit<9> egressSpec_t; //typedef egress
typedef bit<48> macAddr_t; // typedef macaddr
// before ipv4addr
typedef bit<32> ip4Addr_t;
header ethernet_t {
    macAddr_t dstAddr;
    // TypeName: srcAddr
    macAddr_t srcAddr;
    bit<16>   etherType; // inline Type_Bits
}

header ipv4_t {
    bit<4>    version;
    bit<4>    ihl;
    bit<8>    diffserv;
    bit<16>   totalLen;
    bit<16>   identification;
    bit<3>    flags; // <bits> type comment
    bit<13>   fragOffset;
    bit<8>    ttl;
    bit<8>    protocol;
    bit<16>   hdrChecksum;
    ip4Addr_t srcAddr;
    ip4Addr_t dstAddr;
}

struct metadata {
    /* empty */
}

// struct comment
struct headers {
    // struct field comment
    ethernet_t ethernet;
    ipv4_t     ipv4;
}

// free floating

// control block checksum
control MyVerifyChecksum(inout headers hdr, inout metadata meta) {
    apply {  }
}

// control block ingress
control MyIngress( /*argument comment*/ inout headers hdr,
                  inout metadata meta,
                  inout standard_metadata_t standard_metadata) {
    // function call
    action drop() {
        mark_to_drop(standard_metadata);
    }

    action ipv4_forward(macAddr_t dstAddr, egressSpec_t port) {
    }

    table ipv4_lpm {
        key = {
            hdr.ipv4.dstAddr: lpm; // match on dest ipv4
        }
        // action block
        actions = {
            ipv4_forward;
            drop;
            NoAction;
        }
        size = 1024; // table size
        default_action = NoAction();
    }

    // apply fn call
    apply {
        ipv4_lpm.apply(); // apply table
    }
}
