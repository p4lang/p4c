#include <core.p4>

header ethernet_t {
    bit<48> dst_addr;
    bit<48> src_addr;
    bit<16> eth_type;
}

struct Headers {
    ethernet_t eth_hdr;
}

extern bit<16> do_extern(bit<32> val);
parser p(packet_in pkt, out Headers hdr) {
    state start {
        pkt.extract<ethernet_t>(hdr.eth_hdr);
        transition accept;
    }
}

control ingress(inout Headers h) {
    @hidden action gauntlet_extern_arguments_1l28() {
        h.eth_hdr.eth_type = do_extern(32w12);
    }
    @hidden table tbl_gauntlet_extern_arguments_1l28 {
        actions = {
            gauntlet_extern_arguments_1l28();
        }
        const default_action = gauntlet_extern_arguments_1l28();
    }
    apply {
        tbl_gauntlet_extern_arguments_1l28.apply();
    }
}

parser Parser(packet_in b, out Headers hdr);
control Ingress(inout Headers hdr);
package top(Parser p, Ingress ig);
top(p(), ingress()) main;
