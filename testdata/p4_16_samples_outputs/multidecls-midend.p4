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
    @name("ingress.addr") bit<32> addr_0;
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @name("ingress.dummy_action") action dummy_action() {
    }
    @name("ingress.t1") table t1_0 {
        key = {
            addr_0: exact @name("addr");
        }
        actions = {
            dummy_action();
            @defaultonly NoAction_1();
        }
        default_action = NoAction_1();
    }
    @hidden action multidecls35() {
        addr_0 = h.ip_hdr.srcAddr;
    }
    @hidden action multidecls38() {
        addr_0 = h.ip_hdr.dstAddr;
    }
    @hidden table tbl_multidecls35 {
        actions = {
            multidecls35();
        }
        const default_action = multidecls35();
    }
    @hidden table tbl_multidecls38 {
        actions = {
            multidecls38();
        }
        const default_action = multidecls38();
    }
    apply {
        if (h.ip_hdr.protocol > 8w5) {
            tbl_multidecls35.apply();
        } else {
            tbl_multidecls38.apply();
        }
        t1_0.apply();
    }
}

control c<H>(inout H h);
package top<H>(c<H> c);
top<Headers>(ingress()) main;
