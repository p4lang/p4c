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
// and custom extern blocks

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

///////////////////////

header H {
    bit<32> rtt;
}

struct Metadata {
    bit<32> pkt_len;
}

const bit<32> MAX_ALLOWABLE_RTT = 2500;

control ingress(inout H pkt_hdr, in Metadata metadata) {
    Counter<bit<32>>(CounterType.packets_and_bytes) input_traffic_bytes;
    ConditionalAccumulator<bit<32>>(1) sum_rtt_Tr;
    ConditionalAccumulator<bit<32>>(1) num_pkts_with_rtt;

    apply {
        @atomic {
            input_traffic_bytes.count();
            sum_rtt_Tr.write(pkt_hdr.rtt, pkt_hdr.rtt < MAX_ALLOWABLE_RTT);
            num_pkts_with_rtt.write(1, pkt_hdr.rtt < MAX_ALLOWABLE_RTT);
        }
    }
}


top(ingress()) main;
