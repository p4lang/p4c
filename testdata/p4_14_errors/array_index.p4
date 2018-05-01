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
    }
}

header data_t   data[4];

parser start {
    extract(data[next]);
    return ingress;
}

action a(port, val) {
    modify_field(data[port].f1, val);
}

table test1 {
    actions {
        a;
        noop;
    }
}

control ingress {
    apply(test1);
}
