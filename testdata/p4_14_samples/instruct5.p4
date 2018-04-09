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
        b1 : 8;
        b2 : 8;
        b3 : 8;
        more : 8;
    }
}
header data_t data;
header_type data2_t {
    fields {
        x1 : 24;
        more : 8;
    }
}
header data2_t extra[4];

parser start {
    extract(data);
    return select(data.more) {
        0: ingress;
        default: parse_extra;
    }
}
parser parse_extra {
    extract(extra[next]);
    return select(latest.more) {
        0: ingress;
        default: parse_extra;
    }
}

action noop() { }
action push1(x1) {
    push(extra);  // push with implicit size of 1
    extra[0].x1 = x1;
    extra[0].more = data.more;
    data.more = 1;
}
action push2(x1, x2) {
    push(extra, 2);
    extra[0].x1 = x1;
    extra[0].more = 1;
    extra[1].x1 = x2;
    extra[1].more = data.more;
    data.more = 1;
}
action pop1() { 
    data.more = extra[0].more;
    pop(extra, 1);
}


table test1 {
    reads {
        data.f1 : exact;
    }
    actions {
        noop;
        push1;
        push2;
        pop1;
    }
}

action output(port) { standard_metadata.egress_spec = port; }
table output { actions { output; } default_action: output(1); }

control ingress {
    apply(test1);
    apply(output);
}
