/*
Copyright 2016 VMware, Inc.

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

#include <core.p4>

extern Fake {
    Fake();
    void call(in bit<32> data);
}

parser P() {
    bit<32> x = 0;
    Fake() fake;
    state start {
        fake.call(x);
        transition accept;
    }
}

control C() {
    bit<32> x = 0;
    bit<32> y = x + 1;
    Fake() fake;
    apply {
        fake.call(y);
    }
}

parser SimpleParser();
control SimpleControl();
package top(SimpleParser prs, SimpleControl ctrl);
top(P(), C()) main;
