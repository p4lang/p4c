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

struct T {
    bit<1> f;
}

struct S {
    tuple<T, T> f1;
    T           f2;
    bit<1>      z;
}

extern void f<D>(in D data);
control c(inout bit<1> r) {
    S s_0;
    bit<1> tmp;
    apply {
        s_0 = { { { 1w0 }, { 1w1 } }, { 1w0 }, 1w1 };
        f<tuple<T, T>>(s_0.f1);
        tmp = s_0.f2.f & s_0.z;
        r = tmp;
    }
}

control simple(inout bit<1> r);
package top(simple e);
top(c()) main;
