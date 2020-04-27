/*
Copyright 2020 Andy Fingerhut

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
    action act_miss() {
        hdr.ethernet.srcAddr = 0xdeadbeef;
    }
    action act_hit(bit<48> x) {
        hdr.ethernet.srcAddr = x;
    }
    table lpm1 {
        key = {
            hdr.ethernet.dstAddr: lpm;
        }
        actions = {
            act_miss;
            act_hit;
        }
        const entries = {
            0x0a0102030000 &&& 0xffffffffffff: act_hit(1);
            0x0a0000000000 &&& 0xff0000000000: act_hit(2);
            0x000000000000 &&& 0x000000000000: act_hit(3);
        }
        const default_action = act_miss();
    }
    apply {
        lpm1.apply();
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
    }
}

V1Switch(parserImpl(),
         verifyChecksum(),
         ingressImpl(),
         egressImpl(),
         updateChecksum(),
         deparserImpl()) main;
