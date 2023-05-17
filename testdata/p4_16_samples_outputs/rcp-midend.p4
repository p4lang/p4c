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
    @name("ingress.input_traffic_bytes_tmp") bit<32> input_traffic_bytes_tmp_0;
    @name("ingress.sum_rtt_Tr_tmp") bit<32> sum_rtt_Tr_tmp_0;
    @name("ingress.num_pkts_with_rtt_tmp") bit<32> num_pkts_with_rtt_tmp_0;
    @name("ingress.input_traffic_bytes") Register<bit<32>>(32w1) input_traffic_bytes_0;
    @name("ingress.sum_rtt_Tr") Register<bit<32>>(32w1) sum_rtt_Tr_0;
    @name("ingress.num_pkts_with_rtt") Register<bit<32>>(32w1) num_pkts_with_rtt_0;
    @hidden action rcp55() {
        sum_rtt_Tr_0.read(32w0, sum_rtt_Tr_tmp_0);
        sum_rtt_Tr_tmp_0 = sum_rtt_Tr_tmp_0 + pkt_hdr.rtt;
        sum_rtt_Tr_0.write(sum_rtt_Tr_tmp_0, 32w0);
        num_pkts_with_rtt_0.read(32w0, num_pkts_with_rtt_tmp_0);
        num_pkts_with_rtt_tmp_0 = num_pkts_with_rtt_tmp_0 + 32w1;
        num_pkts_with_rtt_0.write(num_pkts_with_rtt_tmp_0, 32w0);
    }
    @hidden action rcp50() {
        input_traffic_bytes_0.read(32w0, input_traffic_bytes_tmp_0);
        input_traffic_bytes_tmp_0 = input_traffic_bytes_tmp_0 + metadata.pkt_len;
        input_traffic_bytes_0.write(input_traffic_bytes_tmp_0, 32w0);
    }
    @hidden table tbl_rcp50 {
        actions = {
            rcp50();
        }
        const default_action = rcp50();
    }
    @hidden table tbl_rcp55 {
        actions = {
            rcp55();
        }
        const default_action = rcp55();
    }
    apply {
        @atomic {
            tbl_rcp50.apply();
            if (pkt_hdr.rtt < 32w2500) {
                tbl_rcp55.apply();
            }
        }
    }
}

top<H, Metadata>(ingress()) main;
