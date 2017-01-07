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

parser p1(out bit<2> z1)(bit<2> a) {
    state start {
        z1 = a;
        transition accept;
    }
}

parser p2(out bit<2> z2)(bit<2> b, bit<2> c) {
    p1(2w0) p1a;
    p1(b)   p1b;
    p1(c)   p1c;

    state start {
        bit<2> x1;
        bit<2> x2;
        bit<2> x3;
        p1a.apply(x1);
        p1b.apply(x2);
        p1c.apply(x3);
        z2 = b | c | x1 | x2 | x3;
        transition accept;
    }
}

parser simple(out bit<2> z);
package m(simple n);
m(p2(2w1, 2w2)) main;
