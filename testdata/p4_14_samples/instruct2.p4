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
        f3 : 32;
        f4 : 32;
        b1 : 8;
        b2 : 8;
        b3 : 8;
        b4 : 8;
    }
}
header data_t data;

parser start {
    extract(data);
    return ingress;
}

action do_xor() { bit_xor(data.b1, data.b2, data.b3); }
action do_and() { bit_and(data.b2, data.b3, data.b4); }
action do_or() { bit_or(data.b4, data.b3, data.b1); }
action do_add() { add(data.b3, data.b1, data.b2); }

table test1 {
    reads {
        data.f1 : exact;
    }
    actions {
        do_add;
        do_and;
        do_or;
        do_xor;
    }
}

control ingress {
    apply(test1);
}
