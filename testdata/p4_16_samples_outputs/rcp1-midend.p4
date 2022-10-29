#include <core.p4>

extern ConditionalAccumulator<T> {
    ConditionalAccumulator(bit<32> size);
    void read(out T value);
    void write(in T value, in bool condition);
}

enum CounterType {
    packets,
    bytes,
    packets_and_bytes
}

extern Counter<T> {
    Counter(CounterType type);
    void count();
}

control proto<P, M>(inout P packet, in M meta);
package top<P, M>(proto<P, M> _p);
header H {
    bit<32> rtt;
}

struct Metadata {
    bit<32> pkt_len;
}

control ingress(inout H pkt_hdr, in Metadata metadata) {
    @name("ingress.input_traffic_bytes") Counter<bit<32>>(CounterType.packets_and_bytes) input_traffic_bytes_0;
    @name("ingress.sum_rtt_Tr") ConditionalAccumulator<bit<32>>(32w1) sum_rtt_Tr_0;
    @name("ingress.num_pkts_with_rtt") ConditionalAccumulator<bit<32>>(32w1) num_pkts_with_rtt_0;
    @hidden action rcp1l61() {
        input_traffic_bytes_0.count();
        sum_rtt_Tr_0.write(pkt_hdr.rtt, pkt_hdr.rtt < 32w2500);
        num_pkts_with_rtt_0.write(32w1, pkt_hdr.rtt < 32w2500);
    }
    @hidden table tbl_rcp1l61 {
        actions = {
            rcp1l61();
        }
        const default_action = rcp1l61();
    }
    apply {
        @atomic {
            tbl_rcp1l61.apply();
        }
    }
}

top<H, Metadata>(ingress()) main;
