/*
Copyright 2013-present Barefoot Networks, Inc.

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

#include <core.p4>

header Ethernet_h {
   bit<48> dstAddr;
   bit<48> srcAddr;
   bit<16> etherType;
}

header Mpls_h {
    bit<20> label;
    bit<3>  tc;
    bit     bos;
    bit<8>  ttl;
}

struct Pkthdr {
   Ethernet_h ethernet;
   Mpls_h[3] mpls_vec;
}

parser X(packet_in b, out Pkthdr p)
{
    state start {
        b.extract(p.ethernet);
        transition select(p.ethernet.etherType) {
           16w0x8847 : parse_mpls;
           16w0x0800 : parse_ipv4;
        }
    }
    state parse_mpls {
         b.extract(p.mpls_vec.next);
         transition select(p.mpls_vec.last.bos) {
            1w0 : parse_mpls; // This creates a loop in the FSM
            1w1 : parse_ipv4;
         }
    }
    state parse_ipv4 { transition accept; }
}
