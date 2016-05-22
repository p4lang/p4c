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
        x1 : 3;
        f1 : 7;
        x2 : 6;
        f2 : 32;
        x3 : 5;
        f3 : 20;
        x4 : 7;
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

action noop() { }
action setb1(val) { modify_field(data.b1, val); }

table test1 {
    reads {
        data.f1 : exact;
        data.f3 : exact;
    }
    actions {
        setb1;
        noop;
    }
}

control ingress {
    apply(test1);
}
