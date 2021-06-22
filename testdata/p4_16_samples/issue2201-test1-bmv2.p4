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

struct metadata_t {
}

header Header {
    bit<4> val1;
    int<3> val2;
    bool val3;
}

struct Structure1 {
    bit<8> val1;
    bit<8> val2;
    bool val3;
}

struct Structure2 {
    int<4> val1;
    Header val2;
}

struct Structure3 {
    bool val1;
    Header val2;
    int<4> val3;
    Header val4;
    Structure1 val5;
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

        Header hdr1 = {3,-1,true};
        log_msg("hdr1 = {}",{hdr1});
        // Output: hdr1 = {'val1': 3, 'val2': -1, 'val3': 1, '$valid$': 1}

        log_msg("list: {}", {{(bit<4>) 2, (int<4>) 3, hdr1}});
        // Output: list: {2, 3, {'a': 10, 'b': 9, 'c': 49, '$valid$': 1}}

        tuple <bit<32>,bool,int<32>> x = {10,true,7};
        log_msg("tuple x = {}", {x});
        // Output: tuple x = {'field': 10, 'field_0': 1, 'field_1': 7}
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
