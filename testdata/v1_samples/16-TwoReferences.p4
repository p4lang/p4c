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

parser start {
    extract(ethernet);
    return ingress;
}

action do_b() { no_op(); }
action do_d() { no_op(); }
action do_e() { no_op(); }

action nop()  { no_op(); }

table A {
    reads {
        ethernet.dstAddr : exact; 
    }
    actions {
        do_b;
        do_d;
        do_e;
    }
}

table B { actions { nop; }}
table C { actions { nop; }}
table D { actions { nop; }}
table E { actions { nop; }}
table F { actions { nop; }}

control ingress {
    apply(A) {
        do_b {
            apply(B);
            apply(C);
            }
        do_d {
            apply(D);
            apply(C);
            }
        do_e {
            apply(E);
            }
    }
    apply(F);
}

control egress {
}
