/*
Copyright 2021 Intel Corporation

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

// Some code copied and pasted from an example by Qinshi Wang on
// p4-spec issue #1000

header H {
    bit<8> x;
}

header_union U {
    H h;
}

H f() {
    H h;
    return h;
}

U g() {
    U u;
    return u;
}

H[1] h() {
    H[1] s;
    return s;
}

struct headers_t {
    ethernet_t    ethernet;
    H h1;
    U u1;
}

struct metadata_t {
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
        if (hdr.ethernet.isValid()) {
            log_msg("hdr.ethernet is valid");
        } else {
            log_msg("hdr.ethernet is invalid");
        }
        // As of this version of p4c, any of the following isValid()
        // calls causes a compiler bug:
        // p4c 1.2.2 (SHA: 37cd30ee9 BUILD: DEBUG)

        // Example:

        // p4test p4-spec-1000-bmv2.p4
        // In file: /home/andy/p4c/frontends/p4/sideEffects.cpp:385
        // Compiler Bug: f();: method on left hand side?

        if (f().isValid()) {
            log_msg("f() returned a valid header");
        } else {
            log_msg("f() returned an invalid header");
        }
        if (g().h.isValid()) {
            log_msg("g() returned a header_union with valid member h");
        } else {
            log_msg("g() returned a header_union with invalid member h");
        }
        if (h()[0].isValid()) {
            log_msg("h() returned a header stack with valid element 0");
        } else {
            log_msg("h() returned a header stack with invalid element 0");
        }

        // The following lines give a compile-time error as of
        // 2021-Dec-20 version of p4c.  That seems reasonable to me,
        // depending upon the outcome of a question on the language
        // spec.

        //f().setValid();
        //g().h.setValid();
        //h()[0].setValid();
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
