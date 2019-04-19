/*
Copyright 2017 VMWare, Inc.

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

#include "v1model.p4"

struct headers_t { bit<8> h; }
struct meta_t { bit<8> unused; }

parser p(packet_in b, out headers_t hdrs, inout meta_t m, inout standard_metadata_t meta)
{
    state start {
        b.extract(hdrs);
        transition accept;
    }
}

control i(inout headers_t hdrs, inout meta_t m, inout standard_metadata_t meta) {
    action a() { meta.egress_spec = 1; }
    apply { a(); }
}

control e(inout headers_t hdrs, inout meta_t m, inout standard_metadata_t meta) {
    apply { }
}

control d1(packet_out packet, in headers_t hdrs) {
    apply { packet.emit(hdrs); }
}

control vc(in headers_t hdrs, inout meta_t meta) { apply { } }
control cc(inout headers_t hdrs, inout meta_t meta) { apply { } }

V1Switch(p(), vc(), i(), e(), cc(), d1()) main;
