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

counter c1 {
    type : packets;
    instance_count : 1024;
}

action count_c1_1() {
    count(c1, 1);
}

action count_c1_2() {
    count(c1, 2);
}

table t1 {
    actions {
        count_c1_1;
    }
}

table t2 {
    actions {
        count_c1_2;
    }
}

control ingress {
    apply(t1);
    apply(t2);
}

control egress {
}
