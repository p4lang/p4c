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


header_type data1_t {
    fields {
        f1 : 128;
    }
}
header_type data2_t {
    fields {
        f2 : 128;
    }
}
header_type data3_t {
    fields {
        f3 : 128;
    }
}
header_type data_t {
    fields {
        f4 : 128;
        b1 : 8;
        b2 : 8;
    }
}
header data1_t data1;
header data2_t data2;
header data3_t data3;
header data_t data;

parser start {
    extract(data1);
    return d2;
}
parser d2 {
    extract(data2);
    return d3;
}
parser d3 {
    extract(data3);
    return more;
}
parser more {
    extract(data);
    return ingress;
}

action noop() { }
action setb1(val, port) {
    modify_field(data.b1, val);
    modify_field(standard_metadata.egress_spec, port);
}

table test1 {
    reads {
        data1.f1 : exact;
        data2.f2 : exact;
        data3.f3 : exact;
    }
    actions {
        setb1;
        noop;
    }
}

control ingress {
    apply(test1);
}
