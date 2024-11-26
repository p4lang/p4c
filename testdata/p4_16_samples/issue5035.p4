/*
Copyright 2024 Intel Corporation

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

header h0_t {
    bit<32> f0;
    bit<32> f1;
}

struct struct_data_t {
    bit<3> f0;
    bit<5> f1;
    bit<8> f2;
}

struct data_t {
    h0_t hdr;
    struct_data_t data;
    bit<32> field;
}

parser SimpleParser(inout data_t d);
package SimpleArch(SimpleParser p);

parser ParserImpl(inout data_t d)
{
    state start {
        log_msg("Flattened hierarchical data: {}", {d});
        transition accept;
    }
}

SimpleArch(ParserImpl()) main;
