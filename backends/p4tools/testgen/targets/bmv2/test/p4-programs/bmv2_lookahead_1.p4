/*
Copyright (c) Intel Corp.

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

header h1 {
    bit<8> f1;
}

struct h {
    h1[3] h;
}

struct m {
    bit<8> h_count;
}

parser MyParser(packet_in b, out h hdrs, inout m meta, inout standard_metadata_t std) {
    state start {
       transition select(b.lookahead<bit<8>>()) {
            1: one;
            2: two;
            4: three;
            default: accept;
        }
    }

    state one {
        b.extract(hdrs.h.next);
        meta.h_count = 1;
        transition accept;
    }

    state two {
        b.extract(hdrs.h.next);
        b.extract(hdrs.h.next);
        meta.h_count = 2;
        transition accept;
    }

    state three {
        b.extract(hdrs.h.next);
        b.extract(hdrs.h.next);
        b.extract(hdrs.h.next);
        meta.h_count = 2;
        transition accept;
    }

    state many {
        meta.h_count = 255;
        transition accept;
    }
}

control MyVerifyChecksum(inout h hdr, inout m meta) {
  apply {}
}
control MyIngress(inout h hdr, inout m meta, inout standard_metadata_t std) {
  apply { }
}
control MyEgress(inout h hdr, inout m meta, inout standard_metadata_t std) {
  apply { }
}

control MyComputeChecksum(inout h hdr, inout m meta) {
  apply {}
}
control MyDeparser(packet_out b, in h hdr) {
  apply { }
}

V1Switch(MyParser(), MyVerifyChecksum(), MyIngress(), MyEgress(), MyComputeChecksum(), MyDeparser()) main;
