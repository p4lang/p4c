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
    bit<8>  op1;
    bit<8>  op2;
    bit<8>  out1;
}

struct headers {
    h1_t h1;
}

struct metadata {
}

parser parserI(packet_in pkt,
               out headers hdr,
               inout metadata meta,
               inout standard_metadata_t stdmeta)
{
    state start {
        pkt.extract(hdr.h1);
        transition accept;
    }
}

control cDoOneOp(inout headers hdr,
                 in bit<8> op)
{
    apply {
        // Comment out the next 4 lines and the p4c compiler error disappears
        if (op == 0x00) {
            // _Uncomment_ the next line and the compiler error disappears
            //hdr.h1.out1 = 7;
        } else
        if (op[7:4] == 1) {
            hdr.h1.out1 = 4;
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
        // Comment out the following line and the error disappears
        do_one_op.apply(hdr, hdr.h1.op2);
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
    }
}

V1Switch(parserI(),
         vc(),
         cIngress(),
         cEgress(),
         uc(),
         DeparserI()) main;
