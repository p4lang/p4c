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

// Test case for issue #199
#include <core.p4>

header ethernet_h {
    bit<48> dst;
    bit<48> src;
    bit<16> type;
}

struct headers_t {
    ethernet_h ethernet;
}

parser Parser(packet_in pkt_in, out headers_t hdr) {
    state start {
        pkt_in.extract(hdr.ethernet);
    }
}

control Deparser(in headers_t hdr, packet_out pkt_out) {
    apply {
        pkt_out.emit(hdr.ethernet);
    }
}

parser P<H>(packet_in pkt, out H hdr);
control D<H>(in H hdr, packet_out pkt);

package S<H>(P<H> p, D<H> d);
S(Parser(), Deparser()) main;
