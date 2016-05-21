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
header_type data2_t {
    fields {
        x1 : 16;
        x2 : 16;
    }
}
header data2_t extra[4];

parser start {
    extract(data);
    return select(data.b1) {
    0x00: parse_extra;
    default: ingress;
    }
}
parser parse_extra {
    extract(extra[next]);
    return select(latest.x1) {
    1 mask 1: parse_extra;
    default: ingress;
    }
}

action noop() { }
action push1() { push(extra, 1); }
action push2() { push(extra, 2); }


table test1 {
    reads {
        data.f1 : exact;
    }
    actions {
        noop;
        push1;
        push2;
    }
}

control ingress {
    apply(test1);
}
