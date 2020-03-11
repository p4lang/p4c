/*
Copyright 2020 Cisco Systems, Inc.

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

typedef bit<48>  EthernetAddress;

header ethernet_t {
    EthernetAddress dstAddr;
    EthernetAddress srcAddr;
    bit<16>         etherType;
}

struct headers_t {
    ethernet_t    ethernet;
}

struct mystruct1_t {
    bit<16> f1;
    bit<8>  f2;
}

struct mystruct2_t {
    mystruct1_t s1;
    bit<16> f3;
    bit<32> f4;
}

struct metadata_t {
    mystruct1_t s1;
    mystruct2_t s2;
}

parser parserImpl(packet_in packet,
                  out headers_t hdr,
                  inout metadata_t meta,
                  inout standard_metadata_t stdmeta)
{
    state start {
        packet.extract(hdr.ethernet);
        transition accept;
    }
}

control verifyChecksum(inout headers_t hdr, inout metadata_t meta) {
    apply { }
}

control ingressImpl(inout headers_t hdr,
                    inout metadata_t meta,
                    inout standard_metadata_t stdmeta)
{
    apply {
        // All of the following lines compile OK, and turn into what
        // looks like correctly transformed P4_16 code in the output
        // of the last MidEnd pass.
        meta.s1 = {f1=2, f2=3};
        meta.s2.s1 = meta.s1;
        meta.s2 = {s1=meta.s1, f3=5, f4=8};

        // I believe the next line should assign the same value to
        // meta.s2 as the last line above.  With p4c 1.2.0 SHA:
        // 62fd40a0 it causes causes the following error message:

        // In file: /home/andy/p4c/frontends/p4/typeChecking/typeChecker.cpp:144
        // Compiler Bug: At this point in the compilation typechecking should not infer new types anymore, but it did.

        meta.s2 = {s1={f1=2, f2=3}, f3=5, f4=8};

        stdmeta.egress_spec = (bit<9>) meta.s2.s1.f2;
    }
}

control egressImpl(inout headers_t hdr,
                   inout metadata_t meta,
                   inout standard_metadata_t stdmeta)
{
    apply { }
}

control updateChecksum(inout headers_t hdr, inout metadata_t meta) {
    apply { }
}

control deparserImpl(packet_out packet,
                     in headers_t hdr)
{
    apply {
        packet.emit(hdr.ethernet);
    }
}

V1Switch(parserImpl(),
         verifyChecksum(),
         ingressImpl(),
         egressImpl(),
         updateChecksum(),
         deparserImpl()) main;
