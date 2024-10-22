/*
Copyright 2024 Cisco Systems, Inc.

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

action foo1() {
}

@name("foo2")
action foo2() {
}

@name(".baz")
action foo3() {
}

control ingressImpl(
    inout headers_t hdr,
    inout metadata_t meta,
    inout standard_metadata_t stdmeta)
{
    bit<8> tmp1;
    bit<8> tmp2;

    // This is not a name conflict with foo1, because it has a top
    // level name ".foo1", but a1 will have hierarchical name
    // "ingressImpl.foo1".
    @name("foo1") action a1 (bit<8> x, bit<8> y) { tmp1 = x >> 1; tmp2 = y; }

    // This should not be a name conflict with foo2, because it has a
    // top level name ".foo2", but a2 will have hierarchical name
    // "ingressImpl.foo2".
    @name("foo2") action a2 (bit<8> x, bit<8> y) { tmp1 = x >> 2; tmp2 = y; }

    @name(".bar") action a3 (bit<8> x, bit<8> y) { tmp1 = x >> 3; tmp2 = y; }
    // This is not a name conflict with a3, because it has a top level
    // name ".bar", but a4 will have hierarchical name
    // "ingressImpl.bar".
    @name("bar") action a4 (bit<8> x, bit<8> y) { tmp1 = x >> 4; tmp2 = y; }

    // This is not a name conflict with foo3, because it has a top
    // level name ".baz", but a5 will have hierarchical name
    // "ingressImpl.baz".
    @name("baz") action a5 (bit<8> x, bit<8> y) { tmp1 = x >> 5; tmp2 = y; }

    table t1 {
        actions = {
            NoAction;
            a1;
            a2;
            a3;
            a4;
            a5;
            foo1;
            foo2;
            foo3;
        }
        key = { hdr.ethernet.etherType: exact; }
        default_action = NoAction();
        size = 512;
    }
    apply {
        tmp1 = hdr.ethernet.srcAddr[7:0];
        tmp2 = hdr.ethernet.dstAddr[7:0];
        t1.apply();
        // This is here simply to ensure that the compiler cannot
        // optimize away the effects of t1 and t2, which can only
        // assign values to variables tmp1 and tmp2.
        hdr.ethernet.etherType = (bit<16>) (tmp1 - tmp2);
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
    }
}

V1Switch(parserImpl(),
         verifyChecksum(),
         ingressImpl(),
         egressImpl(),
         updateChecksum(),
         deparserImpl()) main;
