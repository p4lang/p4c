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
            // There should be no error for any of these lines,
            // because the number of fields in {} (0) matches the
            // number of fields in hdr.h0 and hdr.hstructs.s0.  hdr.h0
            // is initialized to be a valid header with 0 fields.
            hdr.h0 = {};
            hdr.hstructs.s0 = {};
            if (hdr.ethernet.etherType == 0) {
                hdr.h1 = {42};
                hdr.h2 = {43, 44};
                hdr.hstructs.s1 = {5};
                hdr.hstructs.s2 = {5, 10};
            } else {
                hdr.h1 = {f1=52};
                hdr.h2 = {f2=53, f1=54};
                hdr.hstructs.s1 = {f1=6};
                hdr.hstructs.s2 = {f2=11, f1=8};
            }

            // Code that can be used to distinguish whether hdr.h0 was
            // initialized to a valid or invalid header.
            hdr.ethernet.dstAddr = (bit<48>) ((bit<1>) hdr.h0.isValid());
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
