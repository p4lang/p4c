/*
Copyright 2018 Barefoot Networks, Inc.

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

header_type A {
    fields {
        A : 32;
        B : 32;
        h1 : 16;
        b1 : 8;
        b2 : 8;
    }
}
header_type B {
    fields {
        A : 9;
        B : 10;
    }
}
header A A;
metadata B meta;

parser start { return A; }

parser A {
    extract(A);
    return ingress;
}

counter B {
    type: packets_and_bytes;
    instance_count: 1024;
}

action noop() { }
action A(val, port, idx) {
    modify_field(A.b1, val);
    modify_field(standard_metadata.egress_spec, port);
    modify_field(meta.B, idx);
}
action B() { count(B, meta.B); }

table A {
    reads {
        A.A : exact;
    }
    actions {
        A;
        noop;
    }
}

table B {
    actions { B; }
    default_action: B();
}

control ingress {
    apply(A);
    apply(B);
}
