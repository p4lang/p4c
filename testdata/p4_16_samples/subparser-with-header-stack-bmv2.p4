/*
Copyright 2017 Cisco Systems, Inc.

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

header h1_t {
    bit<8>  hdr_type;
    bit<8>  op1;
    bit<8>  op2;
    bit<8>  op3;
    bit<8>  h2_valid_bits;
    bit<8>  next_hdr_type;
}

header h2_t {
    bit<8>  hdr_type;
    bit<8>  f1;
    bit<8>  f2;
    bit<8>  next_hdr_type;
}

header h3_t {
    bit<8>  hdr_type;
    bit<8>  data;
}

#define MAX_H2_HEADERS 5

struct headers {
    h1_t h1;
    h2_t[MAX_H2_HEADERS] h2;
    h3_t h3;
}

struct metadata {
}

error {
    BadHeaderType
}

parser subParserImpl(packet_in pkt,
                     inout headers hdr,
                     out bit<8> ret_next_hdr_type)
{
    state start {
        pkt.extract(hdr.h2.next);
        verify(hdr.h2.last.hdr_type == 2, error.BadHeaderType);
        ret_next_hdr_type = hdr.h2.last.next_hdr_type;
        transition accept;
    }
}

parser parserI(packet_in pkt,
               out headers hdr,
               inout metadata meta,
               inout standard_metadata_t stdmeta)
{
    subParserImpl() subp;
    bit<8> my_next_hdr_type;
    state start {
        pkt.extract(hdr.h1);
        verify(hdr.h1.hdr_type == 1, error.BadHeaderType);
        transition select(hdr.h1.next_hdr_type) {
            2: parse_first_h2;
            3: parse_h3;
            default: accept;
        }
    }
    state parse_first_h2 {
        subp.apply(pkt, hdr, my_next_hdr_type);
        transition select(my_next_hdr_type) {
            2: parse_other_h2;
            3: parse_h3;
            default: accept;
        }
    }
    state parse_other_h2 {
        pkt.extract(hdr.h2.next);
        verify(hdr.h2.last.hdr_type == 2, error.BadHeaderType);
        transition select(hdr.h2.last.next_hdr_type) {
            2: parse_other_h2;
            3: parse_h3;
            default: accept;
        }
    }
    state parse_h3 {
        pkt.extract(hdr.h3);
        verify(hdr.h3.hdr_type == 3, error.BadHeaderType);
        transition accept;
    }
}

control cIngress(inout headers hdr,
                 inout metadata meta,
                 inout standard_metadata_t stdmeta)
{
    apply {
        // Record valid bits of all headers in hdr.h1.h2_valid_bits
        // output header field, so we can easily write unit tests that
        // check whether they have the expected values.
        hdr.h1.h2_valid_bits = 0;
        if (hdr.h2[0].isValid()) {
            hdr.h1.h2_valid_bits[0:0] = 1;
        }
        if (hdr.h2[1].isValid()) {
            hdr.h1.h2_valid_bits[1:1] = 1;
        }
        if (hdr.h2[2].isValid()) {
            hdr.h1.h2_valid_bits[2:2] = 1;
        }
        if (hdr.h2[3].isValid()) {
            hdr.h1.h2_valid_bits[3:3] = 1;
        }
        if (hdr.h2[4].isValid()) {
            hdr.h1.h2_valid_bits[4:4] = 1;
        }
    }
}

control cEgress(inout headers hdr,
                inout metadata meta,
                inout standard_metadata_t stdmeta)
{
    apply { }
}

control vc(inout headers hdr,
           inout metadata meta)
{
    apply { }
}

control uc(inout headers hdr,
           inout metadata meta)
{
    apply { }
}

control DeparserI(packet_out packet,
                  in headers hdr)
{
    apply {
        packet.emit(hdr.h1);
        packet.emit(hdr.h2);
        packet.emit(hdr.h3);
    }
}

V1Switch(parserI(),
         vc(),
         cIngress(),
         cEgress(),
         uc(),
         DeparserI()) main;
