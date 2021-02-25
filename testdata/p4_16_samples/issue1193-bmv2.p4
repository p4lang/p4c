#include <core.p4>
#include <v1model.p4>

extern jnf_counter {
    jnf_counter(CounterType type);
    void count();
}

struct metadata {
}
struct headers {
}

parser MyParser(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    state start {
        transition accept;
    }
}
control MyVerifyChecksum(inout headers hdr, inout metadata meta) {
    apply {
    }
}
control MyIngress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    jnf_counter(CounterType.packets) c;
    action a() {
      c.count();
    }
    table t {
      actions = { a; }
    }
    apply {
      t.apply();
   }
}
control MyEgress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}
control MyDeparser(packet_out packet, in headers hdr) {
    apply {
    }
}
control MyComputeChecksum(inout headers hdr, inout metadata meta) {
    apply {
    }
}
V1Switch(MyParser(), MyVerifyChecksum(), MyIngress(), MyEgress(), MyComputeChecksum(), MyDeparser()) main;
