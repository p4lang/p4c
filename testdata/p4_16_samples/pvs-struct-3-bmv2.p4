#include <v1model.p4>

header data_h {
  bit<32> da;
  bit<32> db;
}

struct my_packet {
  data_h data;
}

struct my_metadata {
  data_h[2] data;
}

struct value_set_t {
    @match(ternary) bit<16> field1;
    @match(lpm)     bit<3>  field2;
    @match(exact)   bit<6>  field3;
    @match(range)   bit<5>  field4;
}

parser MyParser(packet_in b, out my_packet p, inout my_metadata m, inout standard_metadata_t s) {

    value_set<value_set_t>(4) pvs;

    state start {
        b.extract(p.data);
        transition select(p.data.da[15:0], p.data.db[7:5], p.data.db[29:24], p.data.da[30:26]) {
            pvs: accept;
            (0x810, 0x4 &&& 0x6, 0x32 &&& 0x33, 10..20) : foo;
        }
    }

    state foo {
        transition accept;
    }
}

control MyVerifyChecksum(inout my_packet hdr, inout my_metadata meta) {
  apply { }
}


control MyIngress(inout my_packet p, inout my_metadata meta, inout standard_metadata_t s) {
    action set_data() {
    }
    table t {
        actions = { set_data; }
        key = { meta.data[0].da : exact;}
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
  apply { }
}

control MyDeparser(packet_out b, in my_packet p) {
  apply { }
}

V1Switch(MyParser(), MyVerifyChecksum(), MyIngress(), MyEgress(), MyComputeChecksum(), MyDeparser()) main;
