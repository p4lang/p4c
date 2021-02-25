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

#include "core.p4"
#include "v1model.p4"

struct PortId_t { bit<9> _v; }

const PortId_t PSA_CPU_PORT = {9w192};

struct parsed_headers_t {
}

struct metadata_t {
    PortId_t foo;
    PortId_t bar;
}

parser ParserImpl (packet_in packet,
                   out parsed_headers_t hdr,
                   inout metadata_t meta,
                   inout standard_metadata_t standard_metadata)
{
    state start {
        transition accept;
    }
}

control IngressImpl (inout parsed_headers_t hdr,
                     inout metadata_t meta,
                     inout standard_metadata_t standard_metadata)
{
    apply {
        // No problem here.  Comparison between two struct variables
        // seems to compile just fine, even for bmv2 back end.
        if (meta.foo == meta.bar) {
            meta.foo._v = meta.foo._v + 1;
        }

        // Latest p4test and p4c-bm2-ss as of Apr 3, 2018 gives an
        // error for the == comparison below:

        // struct-variable-to-constant-compare-error.p4(58): error: ==: not defined on struct PortId_t and Tuple(1)

        if (meta.foo == PSA_CPU_PORT) {
            meta.foo._v = meta.foo._v + 1;
        }
    }
}

control EgressImpl (inout parsed_headers_t hdr,
                    inout metadata_t meta,
                    inout standard_metadata_t standard_metadata)
{
    apply {
    }
}

control DeparserImpl (packet_out packet,
                      in parsed_headers_t hdr)
{
    apply {
    }
}

control VerifyChecksumImpl (inout parsed_headers_t hdr,
                            inout metadata_t meta)
{
    apply {
    }
}

control ComputeChecksumImpl (inout parsed_headers_t hdr,
                             inout metadata_t meta)
{
    apply {
    }
}

V1Switch(ParserImpl(),
         VerifyChecksumImpl(),
         IngressImpl(),
         EgressImpl(),
         ComputeChecksumImpl(),
         DeparserImpl()) main;
