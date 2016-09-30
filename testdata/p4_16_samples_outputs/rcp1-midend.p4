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
    @name("tmp") bool tmp_1;
    @name("tmp_0") bool tmp_2;
    @name("input_traffic_bytes") Counter<bit<32>>(CounterType.packets_and_bytes) input_traffic_bytes;
    @name("sum_rtt_Tr") ConditionalAccumulator<bit<32>>(32w1) sum_rtt_Tr;
    @name("num_pkts_with_rtt") ConditionalAccumulator<bit<32>>(32w1) num_pkts_with_rtt;
    action act() {
        input_traffic_bytes.count();
        tmp_1 = pkt_hdr.rtt < 32w2500;
        sum_rtt_Tr.write(pkt_hdr.rtt, tmp_1);
        tmp_2 = pkt_hdr.rtt < 32w2500;
        num_pkts_with_rtt.write(32w1, tmp_2);
    }
    table tbl_act() {
        actions = {
            act();
        }
        const default_action = act();
    }
    apply {
        @atomic {
            tbl_act.apply();
        }
    }
}

top<H, Metadata>(ingress()) main;
