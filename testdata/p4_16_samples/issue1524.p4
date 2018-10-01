/*
Copyright 2018 Cisco Systems, Inc.

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
#include <v1model.p4>

// No 'typechecker mutated program' error if you change 'type' to 'typedef'
type bit<7> foo_t;

header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
}

struct metadata {
}

struct headers {
    ethernet_t ethernet;
}

parser ParserImpl(packet_in packet,
                  out headers hdr,
                  inout metadata meta,
                  inout standard_metadata_t standard_metadata)
{
    state start {
        transition parse_ethernet;
    }
    state parse_ethernet {
        packet.extract(hdr.ethernet);
        transition accept;
    }
}

control bar() {
    apply {
        const foo_t FOO = (foo_t) 1;
        // uncommenting this line causes 'typechecker mutated program' error
        foo_t tmp = FOO;

        // no error if only this line is uncommented
        //foo_t tmp = (foo_t) 1;
    }
}
