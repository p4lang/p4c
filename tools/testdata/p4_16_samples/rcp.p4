/*
Copyright 2016 VMware, Inc.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

// Example Rate-Control Protocol implementation using @atomic blocks

#include <core.p4>

extern Register<T> {
    Register(bit<32> size);
    void read(in bit<32> index, out T value);
    void write(in bit<32> index, in T value);
}

control proto<P, M>(inout P packet, in M meta);
package top<P, M>(proto<P, M> _p);

///////////////////////

header H {
    bit<32> rtt;
}

struct Metadata {
    bit<32> pkt_len;
}

const bit<32> MAX_ALLOWABLE_RTT = 2500;

control ingress(inout H pkt_hdr, in Metadata metadata) {
    Register<bit<32>>(1) input_traffic_bytes;
    Register<bit<32>>(1) sum_rtt_Tr;
    Register<bit<32>>(1) num_pkts_with_rtt;

    apply {
        @atomic{
            bit<32> input_traffic_bytes_tmp;
            input_traffic_bytes.read(0, input_traffic_bytes_tmp);
            input_traffic_bytes_tmp = input_traffic_bytes_tmp + metadata.pkt_len;
            input_traffic_bytes.write(input_traffic_bytes_tmp, 0);
            if (pkt_hdr.rtt < MAX_ALLOWABLE_RTT) {
                bit<32> sum_rtt_Tr_tmp;
                sum_rtt_Tr.read(0, sum_rtt_Tr_tmp);
                sum_rtt_Tr_tmp = sum_rtt_Tr_tmp + pkt_hdr.rtt;
                sum_rtt_Tr.write(sum_rtt_Tr_tmp, 0);

                bit<32> num_pkts_with_rtt_tmp;
                num_pkts_with_rtt.read(0, num_pkts_with_rtt_tmp);
                num_pkts_with_rtt_tmp = num_pkts_with_rtt_tmp + 1;
                num_pkts_with_rtt.write(num_pkts_with_rtt_tmp, 0);
            }
        }
    }
}


top(ingress()) main;
