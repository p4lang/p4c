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

parser start {
    extract(data);
    return ingress;
}

action noop() { }
action setb1(val) { modify_field(data.b1, val); }
action setb2(val) { modify_field(data.b2, val); }
action setb3(val) { modify_field(data.b3, val); }
action setb4(val) { modify_field(data.b4, val); }
action setb12(v1, v2) { modify_field(data.b1, v1); modify_field(data.b2, v2); }
action setb13(v1, v2) { modify_field(data.b1, v1); modify_field(data.b3, v2); }
action setb14(v1, v2) { modify_field(data.b1, v1); modify_field(data.b4, v2); }
action setb23(v1, v2) { modify_field(data.b2, v1); modify_field(data.b3, v2); }
action setb24(v1, v2) { modify_field(data.b2, v1); modify_field(data.b4, v2); }
action setb34(v1, v2) { modify_field(data.b3, v1); modify_field(data.b4, v2); }

table test1 {
    reads {
        data.f1 : exact;
    }
    actions {
        noop;
        setb1;
        setb2;
        setb3;
        setb4;
        setb12;
        setb13;
        setb14;
        setb23;
        setb24;
        setb34;
    }
}

control ingress {
    apply(test1);
}
