/*
Copyright 2018 MNK Consulting, LLC.
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

header hdr {
    bit<32> f;
    row_t row;
}

control compute(inout hdr h) {
    apply {
	hdr[1] tmp;
	tmp[0].row.alt1.valid = 1;
	tmp[0].f = h.f + 1;
	h.f = tmp[0].f;
	tmp[0].row.alt0.port = h.row.alt0.port + 1;
	h.row.alt1.valid = tmp[0].row.alt1.valid;
    }
}

#include "arith-inline-skeleton.p4"
