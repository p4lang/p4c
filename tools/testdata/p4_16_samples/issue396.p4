/*
Copyright 2017 VMware, Inc.

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
header H {
    bit<32> x;
}
struct S {
    H h;
}

control c(out bool b);
package top(c _c);

control e(in H h, out bool valid) {
    apply {
        valid = h.isValid();
    }
}

control d(out bool b) {
    e() einst;

    apply {
        H h;
        H h1;
        H[2] h3;
        h = { 0 };

        S s;
        S s1;
        s = { { 0 } };

        s1.h = { 0 };
        h3[0] = { 0 };
        h3[1] = { 1 };

        bool eout;
        einst.apply({ 0 }, eout);
        b = h.isValid() && eout && h3[1].isValid() && s1.h.isValid();
    }
}
top(d()) main;
