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

header_type hdr1_t {
    fields {
        f1 : 8;
        f2 : 8;
    }
}

header hdr1_t hdr1;

parser start {
    extract(hdr1);
    return ingress;
}

action a11() {
    modify_field(standard_metadata.egress_spec, 1);
}

action a12() {
    modify_field(standard_metadata.egress_spec, 2);
}

table t_ingress_1 {
    reads {
        hdr1.f1 : exact;
    }
    actions {
        a11; a12;
    }
    size : 128;
}

control ingress {
    apply(t_ingress_1);
}

control egress {
}
