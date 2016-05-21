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

header_type metadata_t {
    fields {
        field_1_1_1  : 1;
        field_2_1_1  : 1;
    }
}

metadata metadata_t md;

parser start {
    return ingress;
}

#define ACTION(w, n)                                    \
action action_##w##_##n(value) {                        \
    modify_field(md.field_1_##w##_##n, value);          \
    modify_field(md.field_2_##w##_##n, w##n);           \
}

ACTION(1, 1)

table dmac {
    reads {
    }
    actions {
        action_1_1;
    }
}

control ingress {
    apply(dmac);
}

control egress {
}
