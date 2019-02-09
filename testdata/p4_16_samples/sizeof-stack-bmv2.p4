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
    bit<32> len = 0;
    apply {
	hdr[1] tmp;
	len = tmp.sizeBits() + tmp.sizeBits();
	len = tmp.sizeBytes() + tmp.sizeBytes();	
    }
}

#include "arith-inline-skeleton.p4"
