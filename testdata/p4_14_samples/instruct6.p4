/*
Copyright 2013-present Barefoot Networks, Inc. 

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

header_type data_t {
    fields {
        f1 : 32;
        f2 : 32;
        h1 : 16;
        h2 : 16;
        b1 : 8;
        b2 : 8;
    }
}
header data_t data;

parser start {
    extract(data);
    return ingress;
}

action noop() { }
action op1(port) {
    modify_field_with_shift(data.h1, data.h2, 8, 0xff);
    modify_field(standard_metadata.egress_spec, port);
}
action op2(port) {
    modify_field_with_shift(data.h1, data.h2, 4, 0xf0);
    modify_field(standard_metadata.egress_spec, port);
}

table test1 {
    reads { data.f1 : exact; }
    actions { op1; op2; noop; }
}

control ingress {
    apply(test1);
}
