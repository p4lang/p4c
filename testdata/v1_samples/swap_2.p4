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

header_type hdr2_t {
    fields {
        f1 : 8;
        f2 : 8;
        f3 : 16;
    }
}

header hdr2_t hdr2;

parser start {
    extract(hdr2);
    return ingress;
}

action a21() {
    modify_field(standard_metadata.egress_spec, 3);
}

action a22() {
    modify_field(standard_metadata.egress_spec, 4);
}

table t_ingress_2 {
    reads {
        hdr2.f1 : exact;
    }
    actions {
        a21; a22;
    }
    size : 64;
}

control ingress {
    apply(t_ingress_2);
}

control egress {
}
