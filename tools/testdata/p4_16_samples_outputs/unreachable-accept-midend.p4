#include <core.p4>

header ethernet_h {
    bit<48> dst;
    bit<48> src;
    bit<16> type;
}

struct headers_t {
    ethernet_h ethernet;
}

parser Parser(packet_in pkt_in, out headers_t hdr) {
    state start {
        pkt_in.extract<ethernet_h>(hdr.ethernet);
        transition reject;
    }
}

control Deparser(in headers_t hdr, packet_out pkt_out) {
    @hidden action act() {
        pkt_out.emit<ethernet_h>(hdr.ethernet);
    }
    @hidden table tbl_act {
        actions = {
            act();
        }
        const default_action = act();
    }
    apply {
        tbl_act.apply();
    }
}

parser P<H>(packet_in pkt, out H hdr);
control D<H>(in H hdr, packet_out pkt);
package S<H>(P<H> p, D<H> d);
S<headers_t>(Parser(), Deparser()) main;

