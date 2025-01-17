#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

header ethernet_t {
    bit<48> dst_addr;
    bit<48> src_addr;
    bit<16> eth_type;
}

struct Headers {
    ethernet_t eth_hdr;
}

struct Meta {
}

parser p(packet_in pkt, out Headers hdr, inout Meta m, inout standard_metadata_t sm) {
    state start {
        pkt.extract<ethernet_t>(hdr.eth_hdr);
        transition accept;
    }
}

control ingress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    @name("ingress.ret") bit<8> ret_0;
    @name("ingress.x") bit<16> x;
    @name("ingress.hasReturned") bool hasReturned_0;
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @name("ingress.top_a1") action top_a1(@name("y") bit<16> y) {
        x = y;
        hasReturned_0 = false;
        if (h.eth_hdr.eth_type >= x + 16w10) {
            ret_0 = ret_0 + 8w1;
        } else {
            if (h.eth_hdr.eth_type >= x + 16w5) {
                ret_0 = ret_0 + 8w2;
            } else if (h.eth_hdr.eth_type == x) {
                ret_0 = ret_0 + 8w4;
                exit;
            }
            if (hasReturned_0) {
                ;
            } else {
                ret_0 = ret_0 + 8w8;
                h.eth_hdr.src_addr[7:0] = ret_0;
            }
        }
        if (h.eth_hdr.src_addr == h.eth_hdr.dst_addr) {
            ret_0 = ret_0 + 8w16;
            ret_0 = ret_0 + 8w64;
        } else {
            ret_0 = ret_0 + 8w32;
        }
    }
    @name("ingress.t1") table t1_0 {
        key = {
            h.eth_hdr.dst_addr: exact @name("h.eth_hdr.dst_addr");
        }
        actions = {
            top_a1();
            NoAction_1();
        }
        const default_action = NoAction_1();
        size = 32;
    }
    apply {
        ret_0 = 8w0;
        if (h.eth_hdr.isValid()) {
            t1_0.apply();
            h.eth_hdr.src_addr[7:0] = ret_0;
        }
    }
}

control vrfy(inout Headers h, inout Meta m) {
    apply {
    }
}

control update(inout Headers h, inout Meta m) {
    apply {
    }
}

control egress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    apply {
    }
}

control deparser(packet_out pkt, in Headers h) {
    apply {
        pkt.emit<ethernet_t>(h.eth_hdr);
    }
}

V1Switch<Headers, Meta>(p(), vrfy(), ingress(), egress(), update(), deparser()) main;
