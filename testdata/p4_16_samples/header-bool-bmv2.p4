/*
Copyright 2019 MNK Consulting, LLC.
http://mnkcg.com

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

#include <v1model.p4>

header hdr {
    bit<31> f;
    bool    x;
}

struct metadata {
}

parser parserI(packet_in pkt, out hdr h, inout metadata meta,
               inout standard_metadata_t stdmeta)
{
    state start {
        pkt.extract(h);
        transition accept;	
    }
}

control cIngressI(inout hdr h, inout metadata meta,
                  inout standard_metadata_t stdmeta)
{
    apply {
        hdr tmp;
        tmp.x = false;
        h.x = true;
        tmp.f = h.f + 1;
        h.f = tmp.f;
        h.x = tmp.x;
    }
}

control cEgress(inout hdr h, inout metadata meta,
                inout standard_metadata_t stdmeta)
{
    apply { }
}

control vc(inout hdr h,
           inout metadata meta)
{
    apply { }
}

control uc(inout hdr h,
           inout metadata meta)
{
    apply { }
}

control DeparserI(packet_out packet, in hdr h)
{
    apply {
        packet.emit(h);
    }
}

V1Switch(parserI(),
vc(),
cIngressI(),
cEgress(),
uc(),
DeparserI()) main;