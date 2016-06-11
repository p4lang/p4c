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
        x1 : 2;
        pad0 : 3;
        x2 : 2;
        pad1 : 5;
        x3 : 1;
        pad2 : 2;
        skip : 32;
        x4 : 1;
        x5 : 1;
        pad3 : 6;
    }
}
header data_t data;

parser start {
    extract(data);
    return ingress;
}

action noop() { }
action setf4(val) { modify_field(data.f4, val); }

table test1 {
    reads {
        data.f1 : exact;
    }
    actions {
        setf4;
        noop;
    }
}
table test2 {
    reads {
        data.f2 : exact;
    }
    actions {
        setf4;
        noop;
    }
}

control ingress {
    if (data.x1 == 1 and data.x4 == 0) {
        apply(test1);
    } else {
        apply(test2);
    }
}
