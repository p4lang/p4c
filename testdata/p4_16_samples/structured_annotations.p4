/*
Copyright 2020 VMware, Inc.

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

struct headers_t { }

struct metadata_t { }

parser parserImpl(packet_in packet,
                  out headers_t hdr,
                  inout metadata_t meta,
                  inout standard_metadata_t stdmeta)
{
    state start {
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
    action my_drop() {
        mark_to_drop(stdmeta);
    }
    action redirect() {
        stdmeta.egress_spec = stdmeta.ingress_port;
    }
#define TEXT_CONST "hello"
#define NUM_CONST 6
    @MixedExprList[1,TEXT_CONST,true,1==2,5+NUM_CONST]
    @MixedKV[label="text", my_bool=true, int_val=2*3]
    table t {
        key = { }
        actions = {
            @myid[1]
            redirect;
            @myid[2]
            my_drop;
            NoAction;
        }
        default_action = NoAction;
    }
    apply {
        t.apply();
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
        packet.emit(hdr);
    }
}

V1Switch(parserImpl(),
         verifyChecksum(),
         ingressImpl(),
         egressImpl(),
         updateChecksum(),
         deparserImpl()) main;
