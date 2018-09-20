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

struct T { bit f; }

struct S {
    tuple<T, T> f1;
    T f2;
    bit z;
}

struct tuple_0 {
    T field;
    T field_0;
}

extern void f<T>(in T data);

control c(inout bit r) {
    apply {
        S s = { { {0}, {1} }, {0}, 1 };
        f(s.f1);
        f<tuple_0>({{0},{1}});
        r = s.f2.f & s.z;
    }
}

control simple(inout bit r);
package top(simple e);
top(c()) main;
