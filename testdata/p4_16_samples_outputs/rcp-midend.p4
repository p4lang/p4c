#include <core.p4>

extern Register<T> {
    Register(bit<32> size);
    void read(in bit<32> index, out T value);
    void write(in bit<32> index, in T value);
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
    bit<32> input_traffic_bytes_tmp;
    bit<32> sum_rtt_Tr_tmp;
    bit<32> num_pkts_with_rtt_tmp;
    bit<32> tmp_2;
    bit<32> tmp_3;
    bit<32> tmp_4;
    @name("input_traffic_bytes") Register<bit<32>>(32w1) input_traffic_bytes;
    @name("sum_rtt_Tr") Register<bit<32>>(32w1) sum_rtt_Tr;
    @name("num_pkts_with_rtt") Register<bit<32>>(32w1) num_pkts_with_rtt;
    action act() {
        sum_rtt_Tr.read(32w0, tmp_3);
        sum_rtt_Tr_tmp = tmp_3;
        sum_rtt_Tr_tmp = tmp_3 + pkt_hdr.rtt;
        sum_rtt_Tr.write(tmp_3 + pkt_hdr.rtt, 32w0);
        num_pkts_with_rtt.read(32w0, tmp_4);
        num_pkts_with_rtt_tmp = tmp_4;
        num_pkts_with_rtt_tmp = tmp_4 + 32w1;
        num_pkts_with_rtt.write(tmp_4 + 32w1, 32w0);
    }
    action act_0() {
        input_traffic_bytes.read(32w0, tmp_2);
        input_traffic_bytes_tmp = tmp_2;
        input_traffic_bytes_tmp = tmp_2 + metadata.pkt_len;
        input_traffic_bytes.write(tmp_2 + metadata.pkt_len, 32w0);
    }
    table tbl_act {
        actions = {
            act_0();
        }
        const default_action = act_0();
    }
    table tbl_act_0 {
        actions = {
            act();
        }
        const default_action = act();
    }
    apply {
        @atomic {
            tbl_act.apply();
            if (pkt_hdr.rtt < 32w2500) {
                tbl_act_0.apply();
            }
        }
    }
}

top<H, Metadata>(ingress()) main;
