#include <core.p4>

header ethernet_t {
    bit<48> dst_addr;
    bit<48> src_addr;
    bit<16> eth_type;
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

struct Headers {
    ethernet_t eth_hdr;
    ipv4_t     ip_hdr;
}

control ingress(inout Headers h) {
    bit<32> addr;
    bit<32> temp;
    action dummy_action() {
    }
    table t1 {
        key = {
            addr: exact @name("addr");
        }
        actions = {
            dummy_action();
            @defaultonly NoAction();
        }
        default_action = NoAction();
    }
    apply {
        if (h.ip_hdr.protocol > 8w5) {
            addr = h.ip_hdr.srcAddr;
            temp = h.eth_hdr.src_addr[31:0];
        } else {
            addr = h.ip_hdr.dstAddr;
            temp = h.eth_hdr.dst_addr[31:0];
        }
        t1.apply();
    }
}

control c<H>(inout H h);
package top<H>(c<H> c);
top<Headers>(ingress()) main;
