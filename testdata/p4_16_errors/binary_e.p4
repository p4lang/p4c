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
struct S {}
header hdr {}

control p()
{
    apply {
        int<2> a;
        int<4> c;
        bool d;
        bit<2> e;
        bit<4> f;
        hdr[5] stack;

        S g;
        S h;

        c = a[2];   // not an array
        c = stack[d];   // indexing with bool

        f = e & f;  // different width
        d = g == h; // not defined on structs

        d = d < d;  // not defined on bool
        d = a > c;  // different width
    }
}
