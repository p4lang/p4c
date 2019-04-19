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

#include <core.p4>

header H {
    bit<32> a;
    bit<32> b;
}

struct S {
    H h1;
    H h2;
    bit<32> c;
}

parser p() {
    state start {
        S s;
        s.c = 0;
        transition accept;
    }
}

parser empty();
package top(empty e);
top(p()) main;
