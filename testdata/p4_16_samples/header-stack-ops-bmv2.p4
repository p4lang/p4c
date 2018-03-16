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

parser parserI(packet_in pkt,
               out headers hdr,
               inout metadata meta,
               inout standard_metadata_t stdmeta)
{
    state start {
        pkt.extract(hdr.h1);
        verify(hdr.h1.hdr_type == 1, error.BadHeaderType);
        transition select(hdr.h1.next_hdr_type) {
            2: parse_h2;
            3: parse_h3;
            default: accept;
        }
    }
    state parse_h2 {
        pkt.extract(hdr.h2.next);
        verify(hdr.h2.last.hdr_type == 2, error.BadHeaderType);
        transition select(hdr.h2.last.next_hdr_type) {
            2: parse_h2;
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

control cDoOneOp(inout headers hdr,
                 in bit<8> op)
{
    apply {
        if (op == 0x00) {
            // nop
        } else if (op[7:4] == 1) {
            // push_front
            if (op[3:0] == 1) {
                hdr.h2.push_front(1);
            } else if (op[3:0] == 2) {
                hdr.h2.push_front(2);
            } else if (op[3:0] == 3) {
                hdr.h2.push_front(3);
            } else if (op[3:0] == 4) {
                hdr.h2.push_front(4);
            } else if (op[3:0] == 5) {
                hdr.h2.push_front(5);
            } else if (op[3:0] == 6) {
                hdr.h2.push_front(6);
            }
        } else if (op[7:4] == 2) {
            // pop_front
            if (op[3:0] == 1) {
                hdr.h2.pop_front(1);
            } else if (op[3:0] == 2) {
                hdr.h2.pop_front(2);
            } else if (op[3:0] == 3) {
                hdr.h2.pop_front(3);
            } else if (op[3:0] == 4) {
                hdr.h2.pop_front(4);
            } else if (op[3:0] == 5) {
                hdr.h2.pop_front(5);
            } else if (op[3:0] == 6) {
                hdr.h2.pop_front(6);
            }
        } else if (op[7:4] == 3) {
            // setValid
            if (op[3:0] == 0) {
                hdr.h2[0].setValid();
                hdr.h2[0].hdr_type = 2;
                hdr.h2[0].f1 = 0xa0;
                hdr.h2[0].f2 = 0x0a;
                hdr.h2[0].next_hdr_type = 9;
            } else if (op[3:0] == 1) {
                hdr.h2[1].setValid();
                hdr.h2[1].hdr_type = 2;
                hdr.h2[1].f1 = 0xa1;
                hdr.h2[1].f2 = 0x1a;
                hdr.h2[1].next_hdr_type = 9;
            } else if (op[3:0] == 2) {
                hdr.h2[2].setValid();
                hdr.h2[2].hdr_type = 2;
                hdr.h2[2].f1 = 0xa2;
                hdr.h2[2].f2 = 0x2a;
                hdr.h2[2].next_hdr_type = 9;
            } else if (op[3:0] == 3) {
                hdr.h2[3].setValid();
                hdr.h2[3].hdr_type = 2;
                hdr.h2[3].f1 = 0xa3;
                hdr.h2[3].f2 = 0x3a;
                hdr.h2[3].next_hdr_type = 9;
            } else if (op[3:0] == 4) {
                hdr.h2[4].setValid();
                hdr.h2[4].hdr_type = 2;
                hdr.h2[4].f1 = 0xa4;
                hdr.h2[4].f2 = 0x4a;
                hdr.h2[4].next_hdr_type = 9;
            }
        } else if (op[7:4] == 4) {
            // setInvalid
            if (op[3:0] == 0) {
                hdr.h2[0].setInvalid();
            } else if (op[3:0] == 1) {
                hdr.h2[1].setInvalid();
            } else if (op[3:0] == 2) {
                hdr.h2[2].setInvalid();
            } else if (op[3:0] == 3) {
                hdr.h2[3].setInvalid();
            } else if (op[3:0] == 4) {
                hdr.h2[4].setInvalid();
            }
        }
    }
}

control cIngress(inout headers hdr,
                 inout metadata meta,
                 inout standard_metadata_t stdmeta)
{
    cDoOneOp() do_one_op;
    apply {
        do_one_op.apply(hdr, hdr.h1.op1);
        do_one_op.apply(hdr, hdr.h1.op2);
        do_one_op.apply(hdr, hdr.h1.op3);

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
