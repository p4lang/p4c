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
            // There should be an error for each of the following two
            // lines, because the number of fields in the list
            // expressions does not match the number of fields in the
            // header or struct they are being assigned to.
            hdr.h1 = {};
            hdr.h2 = {};
            hdr.h0 = {4};
            hdr.h1 = {1, 2};
            hdr.h2 = {3};
            hdr.hstructs.s0 = {1};
            hdr.hstructs.s1 = {1, 2};
            hdr.hstructs.s2 = {3};

            // There should be an error for the following lines,
            // because hdr.h1 and hdr.hstructs.s1 have no field named
            // f2.
            hdr.h1 = {f2=5, f1=2};
            hdr.h1 = {f2=5};
            hdr.hstructs.s1 = {f2=5, f1=2};
            hdr.hstructs.s1 = {f2=5};

            // There should be an error for the following lines,
            // because not all fields of hdr.h2 nor hdr.hstructs.s2
            // are given a value in the structure-valued expression.
            hdr.h2 = {f2=5};
            hdr.hstructs.s2 = {f2=5};
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
        pkt.emit(hdr.hstructs);
    }
}

V1Switch(parserImpl(),
         verifyChecksum(),
         ingressImpl(),
         egressImpl(),
         updateChecksum(),
         deparserImpl()) main;
