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

extern Generic<T> {
    Generic(T sz);
    R get<R>();
    R get1<R, S>(in S value, in R data);
}

extern void f<T>(in T arg);

control c<T>()(T size) {
    Generic<T>(size) x;
    apply {
        bit<32> a = x.get<bit<32>>();
        bit<5> b = x.get1(10w0, 5w0);
        f(b);
    }
}

control caller() {
    c(8w9) cinst;
    apply {
        cinst.apply();
    }
}

control s();
package p(s parg);
p(caller()) main;
