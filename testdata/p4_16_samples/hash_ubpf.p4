/*
Copyright 2019 Orange

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
#define UBPF_MODEL_VERSION 20200515
#include <ubpf_model.p4>

header test_t {
    bit<16> sa;
    bit<8> da;
}

header test1_t {
    bit<8> a;
    bit<16> b;
    bit<16> c;
    bit<8> d;
}

header test2_t {
    bit<8> a;
}

struct Headers_t {
    test_t test;
    test1_t test1;
    test2_t test2;
}

struct metadata {
    bit<32> output;
}

parser prs(packet_in p, out Headers_t headers, inout metadata meta, inout standard_metadata std_meta) {
    state start {
        transition accept;
    }
}

control pipe(inout Headers_t headers, inout metadata meta, inout standard_metadata std_meta) {

    action a() {
        hash(meta.output, HashAlgorithm.lookup3, { headers.test.sa, headers.test.da });
    }

    action b() {
        hash(meta.output, HashAlgorithm.lookup3, { headers.test1.a, headers.test1.b,
                                                   headers.test1.c, headers.test1.d });
    }

    action c() {
        hash(meta.output, HashAlgorithm.lookup3, { headers.test2.a });
    }

    apply {
        a();
        b();
        c();
    }
}

control dprs(packet_out packet, in Headers_t headers) {
    apply { }
}

ubpf(prs(), pipe(), dprs()) main;