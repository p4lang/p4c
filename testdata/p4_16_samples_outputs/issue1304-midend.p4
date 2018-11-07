#include <core.p4>
#include <v1model.p4>

package Pipeline<H, M>(Parser<H, M> p, Ingress<H, M> ig, Egress<H, M> eg, Deparser<H> dp);
package Switch<H0, M0, H1, M1>(Pipeline<H0, M0> p0, @optional Pipeline<H1, M1> p1);
header data_h {
    bit<24> da;
    bit<8>  db;
}

struct my_packet {
    data_h hdr;
}

struct my_metadata {
    bit<32> data;
    error   err;
}

parser MyParser(packet_in b, out my_packet p, inout my_metadata m, inout standard_metadata_t s) {
    state start {
        b.extract<data_h>(p.hdr);
        transition select(p.hdr.da) {
            default: accept;
        }
    }
}

control MyIngress(inout my_packet p, inout my_metadata meta, inout standard_metadata_t s) {
    @name(".NoAction") action NoAction_0() {
    }
    @name("MyIngress.set_data") action set_data() {
    }
    @name("MyIngress.t") table t_0 {
        actions = {
            set_data();
            @defaultonly NoAction_0();
        }
        key = {
            meta.err: exact @name("meta.err") ;
        }
        default_action = NoAction_0();
    }
    apply {
        t_0.apply();
    }
}

control MyEgress(inout my_packet p, inout my_metadata m, inout standard_metadata_t s) {
    apply {
    }
}

control MyDeparser(packet_out b, in my_packet p) {
    apply {
    }
}

Pipeline<my_packet, my_metadata>(MyParser(), MyIngress(), MyEgress(), MyDeparser()) p0;

Switch<my_packet, my_metadata, _, _>(p0) main;

