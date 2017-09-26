/*
Copyright 2017 VMware, Inc.

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
typedef bit<32>  IPv4Address;

struct Headers {
}

struct Key {
	bit<32> field1;
}

struct Value {
	bit<32> field1;
}

struct Metadata {
}

parser P(packet_in b,
         out Headers p,
         inout Metadata meta,
         inout standard_metadata_t standard_meta) {
    state start {
        transition accept;
    }
}

control Ing(inout Headers headers,
            inout Metadata meta,
            inout standard_metadata_t standard_meta) {

    apply {}
}

control Eg(inout Headers hdrs,
           inout Metadata meta,
           inout standard_metadata_t standard_meta) {


    action update(inout Value val) {
        val.field1 = 32w8;
    }

    action test() {
        Key inKey = {1};
        Key defaultKey = {0};

        bool same = (inKey == defaultKey);
        Value val = {0};
        bool done = false;
        bool ok = !done && same;
        if (ok) {
            update(val);
        }
    }

    apply {
		test();
    }
}

control DP(packet_out b, in Headers p) {
    apply {}
}

control Verify(inout Headers hdrs, inout Metadata meta) {
    apply {}
}

control Compute(inout Headers hdr, inout Metadata meta) {
    apply {}
}

V1Switch(P(),
         Verify(),
         Ing(),
         Eg(),
         Compute(),
         DP()) main;
