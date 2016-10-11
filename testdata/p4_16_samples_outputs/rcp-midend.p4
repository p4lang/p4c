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
    bit<32> tmp_9;
    bit<32> tmp_10;
    bit<32> tmp_11;
    bit<32> tmp_12;
    bit<32> tmp_13;
    bit<32> tmp_14;
    bit<32> tmp_15;
    bit<32> tmp_16;
    bit<32> tmp_17;
    bool tmp_18;
    @name("input_traffic_bytes") Register<bit<32>>(32w1) input_traffic_bytes;
    @name("sum_rtt_Tr") Register<bit<32>>(32w1) sum_rtt_Tr;
    @name("num_pkts_with_rtt") Register<bit<32>>(32w1) num_pkts_with_rtt;
    action act() {
        tmp_12 = 32w0;
        sum_rtt_Tr.read(tmp_12, tmp_13);
        sum_rtt_Tr_tmp = tmp_13;
        tmp_14 = sum_rtt_Tr_tmp + pkt_hdr.rtt;
        sum_rtt_Tr_tmp = tmp_14;
        sum_rtt_Tr.write(sum_rtt_Tr_tmp, 32w0);
        tmp_15 = 32w0;
        num_pkts_with_rtt.read(tmp_15, tmp_16);
        num_pkts_with_rtt_tmp = tmp_16;
        tmp_17 = num_pkts_with_rtt_tmp + 32w1;
        num_pkts_with_rtt_tmp = tmp_17;
        num_pkts_with_rtt.write(num_pkts_with_rtt_tmp, 32w0);
    }
    action act_0() {
        tmp_9 = 32w0;
        input_traffic_bytes.read(tmp_9, tmp_10);
        input_traffic_bytes_tmp = tmp_10;
        tmp_11 = input_traffic_bytes_tmp + metadata.pkt_len;
        input_traffic_bytes_tmp = tmp_11;
        input_traffic_bytes.write(input_traffic_bytes_tmp, 32w0);
        tmp_18 = pkt_hdr.rtt < 32w2500;
    }
    table tbl_act() {
        actions = {
            act_0();
        }
        const default_action = act_0();
    }
    table tbl_act_0() {
        actions = {
            act();
        }
        const default_action = act();
    }
    apply {
        @atomic {
            tbl_act.apply();
            if (tmp_18) {
                tbl_act_0.apply();
            }
        }
    }
}

top<H, Metadata>(ingress()) main;
