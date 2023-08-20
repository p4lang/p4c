/*
Copyright 2022 Intel Corporation

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

header h0_t {
}

header h1_t {
    bit<8> f1;
}

header h2_t {
    bit<8> f1;
    bit<8> f2;
}

struct s0_t {
}

struct s1_t {
    bit<8> f1;
}

struct s2_t {
    bit<8> f1;
    bit<8> f2;
}

header hstructs_t {
    s0_t   s0;
    s1_t   s1;
    s2_t   s2;
}

struct headers_t {
    ethernet_t    ethernet;
    h0_t          h0;
    h1_t          h1;
    h2_t          h2;
    hstructs_t    hstructs;
}

struct metadata_t {
}

parser parserImpl(
    packet_in pkt,
    out headers_t hdr,
    inout metadata_t meta,
    inout standard_metadata_t stdmeta)
{
    state start {
        pkt.extract(hdr.ethernet);
        transition accept;
    }
}

control verifyChecksum(
    inout headers_t hdr,
    inout metadata_t meta)
{
    apply { }
}

control ingressImpl(
    inout headers_t hdr,
    inout metadata_t meta,
    inout standard_metadata_t stdmeta)
{
    apply {
        if (hdr.ethernet.isValid()) {
            hdr.hstructs.setValid();
            // This should be an error because it should be an error
            // for a structure-valued expression to assign a value to
            // the same field multiple times.
            hdr.h2 = {f2=5, f1=2, f2=5};
            hdr.hstructs.s2 = {f2=5, f1=2, f2=5};

            // p4c gives an error for the following line because it
            // contains a mix of some values, and some "name=value"
            // elements, in the list expression.  This is not defined
            // in the P4_16 specification.
            hdr.h2 = {2, f2=5};
            hdr.hstructs.s2 = {2, f2=5};
        }
    }
}

control egressImpl(
    inout headers_t hdr,
    inout metadata_t meta,
    inout standard_metadata_t stdmeta)
{
    apply { }
}

control updateChecksum(
    inout headers_t hdr,
    inout metadata_t meta)
{
    apply { }
}

control deparserImpl(
    packet_out pkt,
    in headers_t hdr)
{
    apply {
        pkt.emit(hdr.ethernet);
        pkt.emit(hdr.h0);
        pkt.emit(hdr.h1);
        pkt.emit(hdr.h2);
    }
}

V1Switch(parserImpl(),
         verifyChecksum(),
         ingressImpl(),
         egressImpl(),
         updateChecksum(),
         deparserImpl()) main;
