#include <core.p4>
#include <v1model.p4>

package Pipeline<H, M>(Parser<H, M> p, Ingress<H, M> ig, Egress<H, M> eg, Deparser<H> dp);
package Switch<H0, M0, H1, M1>(Pipeline<H0, M0> p0, @optional Pipeline<H1, M1> p1);
const bit<8> CONST_VAL = 8w10;
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
        b.extract(p.hdr);
        transition select(p.hdr.da) {
            default: accept;
        }
    }
}

control MyVerifyChecksum(inout my_packet hdr, inout my_metadata meta) {
    apply {
    }
}

control MyIngress(inout my_packet p, inout my_metadata meta, inout standard_metadata_t s) {
    action set_data() {
    }
    table t {
        actions = {
            set_data;
        }
        key = {
            meta.err: exact;
        }
    }
    apply {
        t.apply();
    }
}

control MyEgress(inout my_packet p, inout my_metadata m, inout standard_metadata_t s) {
    apply {
    }
}

control MyComputeChecksum(inout my_packet p, inout my_metadata m) {
    apply {
    }
}

control MyDeparser(packet_out b, in my_packet p) {
    apply {
    }
}

Pipeline(MyParser(), MyIngress(), MyEgress(), MyDeparser()) p0;

Switch(p0) main;

