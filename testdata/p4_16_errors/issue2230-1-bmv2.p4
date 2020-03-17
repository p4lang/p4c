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

header h1_t {
    bit<8> f1;
    bit<8> f2;
}

header h2_t {
    bit<8> f1;
    bit<8> f2;
}

struct s1_t {
    bit<8> f1;
    bit<8> f2;
}

struct s2_t {
    bit<8> f1;
    bit<8> f2;
}

struct headers_t {
    ethernet_t    ethernet;
    h1_t h1;
    h2_t h2;
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

s2_t funky_swap(in s2_t s2)
{
    s2_t ret;
    ret = s2;
    ret.f1 = ret.f1 ^ ret.f2;
    ret.f2 = ret.f1 ^ ret.f2;
    ret.f1 = ret.f1 ^ ret.f2;
    return ret;
}

control myCtrl(inout h2_t h2, inout s2_t s2)
{
    bit<8> tmp;
    apply {
        tmp = h2.f1;
        h2.f1 = h2.f2 - 7;
        h2.f2 = tmp + 7;
        tmp = s2.f1;
        s2.f1 = s2.f2 << 3;
        s2.f2 = tmp >> 3;
    }
}

control ingressImpl(inout headers_t hdr,
                    inout metadata_t meta,
                    inout standard_metadata_t stdmeta)
{
    s1_t s1;
    myCtrl() c1;

    apply {
        hdr.h1 = {hdr.ethernet.dstAddr[47:40], hdr.ethernet.dstAddr[39:32]};
        s1 = {hdr.ethernet.dstAddr[31:24], hdr.ethernet.dstAddr[23:16]};

        log_msg("Near start of ingress:");
        log_msg("(bit<1>) hdr.h1.isValid()={} .f1={} .f2={}",
            {(bit<1>) hdr.h1.isValid(), hdr.h1.f1, hdr.h1.f2});
        log_msg("s1 .f1={} .f2={}",
            {s1.f1, s1.f2});

        // Should this apply call cause a compile-time error, because
        // hdr.h1 has a different type h1_t than the first run-time
        // parameter of myCtrl, which is h2_t?
        c1.apply(hdr.h1, s1);

        // I thought yes, since the headers have different types, even
        // though those types have the same number of fields, and
        // field names.

        // bmv2 simple_switch gives _no_ run-time error when executing
        // the Bmv2 JSON produced by this version of p4c:

        // $ p4c --version
        // p4c 1.2.0 (SHA: a1f21cb9)

        log_msg("After c1.apply:");
        log_msg("(bit<1>) hdr.h1.isValid()={} .f1={} .f2={}",
            {(bit<1>) hdr.h1.isValid(), hdr.h1.f1, hdr.h1.f2});
        log_msg("s1 .f1={} .f2={}",
            {s1.f1, s1.f2});

        // Similarly this seems like it should give a compile-time
        // error, because of type name mismatch.  Again, no
        // compile-time error or warning from p4c, and no run-time
        // error when simple_switch executes the code generated.
        s1 = funky_swap(s1);

        log_msg("After funky_swap call:");
        log_msg("s1 .f1={} .f2={}",
            {s1.f1, s1.f2});

        stdmeta.egress_spec = 1;
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
        packet.emit(hdr.h1);
        packet.emit(hdr.h2);
    }
}

V1Switch(parserImpl(),
         verifyChecksum(),
         ingressImpl(),
         egressImpl(),
         updateChecksum(),
         deparserImpl()) main;
