/*
Copyright 2022-present Orange
Copyright 2022-present Open Networking Foundation

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
#include <psa.p4>
#include "common_headers.p4"

struct headers {
    ethernet_t eth;
}

parser MyIP(packet_in buffer, out headers hdr, inout empty_t bp,
            in psa_ingress_parser_input_metadata_t c, in empty_t d, in empty_t e) {
    state start {
        buffer.extract(hdr.eth);
        transition accept;
    }
}

parser MyEP(packet_in buffer, out empty_t a, inout empty_t b,
            in psa_egress_parser_input_metadata_t c, in empty_t d, in empty_t e, in empty_t f) {
    state start {
        transition accept;
    }
}

control MyIC(inout headers a, inout empty_t bc,
             in psa_ingress_input_metadata_t c, inout psa_ingress_output_metadata_t ostd) {

    ActionSelector(PSA_HashAlgorithm_t.CRC32, 32w1024, 32w16) as;
    
    action fwd(PortId_t port) { send_to_port(ostd, port); }

    table tbl {
        key = {
            a.eth.srcAddr : exact;
            a.eth.dstAddr : selector;
        }
        actions = { NoAction; fwd; }
        psa_implementation = as;
    }

    apply {
        tbl.apply();
    }
}

control MyEC(inout empty_t a, inout empty_t b,
    in psa_egress_input_metadata_t c, inout psa_egress_output_metadata_t d) {
    apply { }
}

control MyID(packet_out buffer, out empty_t a, out empty_t b, out empty_t c,
    inout headers d, in empty_t e, in psa_ingress_output_metadata_t f) {
    apply {
        buffer.emit(d.eth);
    }
}

control MyED(packet_out buffer, out empty_t a, out empty_t b, inout empty_t c, in empty_t d,
    in psa_egress_output_metadata_t e, in psa_egress_deparser_input_metadata_t f) {
    apply { }
}

IngressPipeline(MyIP(), MyIC(), MyID()) ip;
EgressPipeline(MyEP(), MyEC(), MyED()) ep;

PSA_Switch(ip, PacketReplicationEngine(), ep, BufferingQueueingEngine()) main;
