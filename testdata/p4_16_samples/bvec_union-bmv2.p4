/*
Copyright 2018 MNK Consulting LLC.
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

struct alt_t {
  bit<1> valid;
  bit<7> port;
};

struct row_t {
  alt_t alt0;
  alt_t alt1;
};

header Hdr1 {
    bit<8> a;
    row_t row0;
    row_t row1;    
}

header Hdr2 {
    bit<16> b;
    row_t row;
}

header_union U {
    Hdr1 h1;
    Hdr2 h2;
}

struct Headers {
    Hdr1 h1;
    U u;
}

struct Meta {}

parser p(packet_in b, out Headers h, inout Meta m, inout standard_metadata_t sm) {
    state start {        
        b.extract(h.h1);
        transition select(h.h1.a) {
            0: getH1;
            default: getH2;
        }
    }

    state getH1 {
        b.extract(h.u.h1);
        transition accept;
    }

    state getH2 {
        b.extract(h.u.h2);
        transition accept;
    }
}

control vrfy(inout Headers h, inout Meta m) { apply {} }
control update(inout Headers h, inout Meta m) { apply {} }

control egress(inout Headers h, inout Meta m, inout standard_metadata_t sm) { apply {} }

control deparser(packet_out b, in Headers h) {
    apply {
        b.emit(h.h1);
        b.emit(h.u);
    }
}

control ingress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    apply {
        if (h.u.h2.isValid())
            h.u.h2.setInvalid();
    }
}

V1Switch(p(), vrfy(), ingress(), egress(), update(), deparser()) main;
