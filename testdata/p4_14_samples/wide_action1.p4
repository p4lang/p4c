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

header_type metadata_t {
    fields {
        m0 : 32;
        m1 : 32;
        m2 : 32;
        m3 : 32;
        m4 : 32;
        m5 : 16;
        m6 : 16;
        m7 : 16;
        m8 : 16;
        m9 : 16;
        m10 : 8;
        m11 : 8;
        m12 : 8;
        m13 : 8;
        m14 : 8;
    }
}
metadata metadata_t m;

parser start {
    extract(data);
    return ingress;
}

action setmeta(v0, v1, v2, v3, v4, v5, v6) {
    modify_field(m.m0, v0);
    modify_field(m.m1, v1);
    modify_field(m.m2, v2);
    modify_field(m.m3, v3);
    modify_field(m.m4, v4);
    modify_field(m.m5, v5);
    modify_field(m.m6, v6);
}

table test1 {
    reads {
        data.f1 : exact;
    }
    actions {
        setmeta;
    }
}

control ingress {
    apply(test1);
}
