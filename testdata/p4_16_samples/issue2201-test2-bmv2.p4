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
    bit<7> val1;
    bool val2;
}

struct Structure1 {
    bit<8> val1;
    bit<8> val2;
    bool val3;
}

struct Structure2 {
    int<7> val1;
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

        Structure1 str1 = {2,3,false};
        log_msg("str1 = {}",{str1});
        // Output: str1 = {'val1': 2, 'val2': 3, 'val3': 0}

        Header hdr1 = {4,true};

        Structure2 str2 = {3,hdr1};
        log_msg("str2 = {}",{str2});
        // Output: str2 = {3, {'val1': 4, 'val2': 1, '$valid$': 1}}

        Header hdr2 = {3,false};

        Structure3 str3 = {true,hdr1,-2,hdr2,str1};
        log_msg("str3 = {}",{str3});
        // Output: str3 = {1, {'val1': 4, 'val2': 1, '$valid$': 1}, -2, {'val1': 3, 'val2': 0, '$valid$': 1}, {'val1': 2, 'val2': 3, 'val3': 0}}
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
