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

header_type vag_t {
    fields {
        f1 : 8;
        f2 : 16;
        f3 : 32;
    }
}

header vag_t vag;

parser start {
    extract(vag);
    return ingress;
}

header_type ingress_metadata_t {
    fields {
        f1 : 8;
        f2 : 16;
        f3 : 32;
    }
} 

metadata ingress_metadata_t ing_metadata;

action nop() {
}

action set_f1(f1) {
    modify_field(ing_metadata.f1, f1);
}

action set_f2(f2) {
    modify_field(ing_metadata.f2, f2);
}

action set_f3(f3) {
    modify_field(ing_metadata.f3, f3);
}

table i_t1 {
    reads {
        vag.f1 : exact;
    }
    actions {
        nop;
        set_f1;
    }
    size : 1024;
}

table i_t2 {
    reads {
        vag.f2 : exact;
    }
    actions {
        nop;
        set_f2;
    }
    size : 1024;
}

table i_t3 {
    reads {
        vag.f3 : exact;
    }
    actions {
        nop;
        set_f3;
    }
    size : 1024;
}

control ingress {
        apply(i_t1);
//        apply(i_t2);
//        apply(i_t3);
}

table e_t1 {
    reads {
        vag.f1 : exact;
    }
    actions {
        nop;
    }
}

control egress {
    apply(e_t1);
}
