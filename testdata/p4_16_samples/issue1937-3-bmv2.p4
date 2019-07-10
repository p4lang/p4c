/*
Copyright 2019 Cisco Systems, Inc.

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

header h1_t { bit<8> f1; bit<8> f2; }
struct headers_t { h1_t h1; }
struct metadata_t { }

control foo (out bit<8> x, in bit<8> y = 5) {
    apply {
        x = (y >> 2);
    }
}

control ingressImpl(inout headers_t hdr, inout metadata_t meta, inout standard_metadata_t stdmeta) {
    apply {
        foo.apply(hdr.h1.f1, hdr.h1.f1);
        foo.apply(hdr.h1.f2);
    }
}
parser parserImpl(packet_in packet, out headers_t hdr, inout metadata_t meta, inout standard_metadata_t stdmeta) { state start { transition accept; } }
control verifyChecksum(inout headers_t hdr, inout metadata_t meta) { apply { } }
control egressImpl(inout headers_t hdr, inout metadata_t meta, inout standard_metadata_t stdmeta) { apply { } }
control updateChecksum(inout headers_t hdr, inout metadata_t meta) { apply { } }
control deparserImpl(packet_out packet, in headers_t hdr) { apply { } }
V1Switch(parserImpl(), verifyChecksum(), ingressImpl(), egressImpl(), updateChecksum(), deparserImpl()) main;
