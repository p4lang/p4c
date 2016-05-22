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

header_type ethernet_t {
    fields {
        dstAddr   : 48;
        srcAddr   : 48;
        ethertype : 16;
    }
}

header ethernet_t ethernet;

header_type vag_t {
    fields {
        f1 : 8;
        f2 : 16;
        f3 : 32;
        f4 : 64;
    }
}

header vag_t vag;

parser start {
    extract(ethernet);
    extract(vag);
    return ingress;
}

header_type ingress_metadata_t {
    fields {
        drop        : 1;
        egress_port : 8;
        f1 : 8;
        f2 : 16;
        f3 : 32;
        f4 : 64;
         
    }
} 

metadata ingress_metadata_t ing_metadata;

action nop() {
}

action ing_drop() {
    modify_field(ing_metadata.drop, 1);
}

#define ING_METADATA_ACTION(f)         \
action set_##f (f) {                   \
    modify_field(ing_metadata.f, f);   \
}

ING_METADATA_ACTION(egress_port)
ING_METADATA_ACTION(f1)
ING_METADATA_ACTION(f2)
ING_METADATA_ACTION(f3)
ING_METADATA_ACTION(f4)

table i_t1 {
    reads {
        vag.f1 : exact;
    }
    actions {
        nop;
        ing_drop;
        set_egress_port;
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

table i_t4 {
    reads {
        vag.f4 : exact;
    }
    actions {
        nop;
        set_f4;
    }
    size : 1024;
}

control ingress {
    apply(i_t1);
    if (ing_metadata.f1 == vag.f1) {
        apply(i_t2);
        if (ing_metadata.f2 == vag.f2) {
            apply(i_t3);
        }
    } else {
        apply(i_t4);
    }
}

table e_t1 {
    reads {
        ethernet.srcAddr : exact;
    }
    actions {
        nop;
    }
}

control egress {
    apply(e_t1);
}
