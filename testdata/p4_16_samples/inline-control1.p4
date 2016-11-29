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

extern Y {
    Y(bit<32> b);
    bit<32> get();
}

control c(out bit<32> x)(Y y) {
    apply {
        x = y.get();
    }
}

control d(out bit<32> x) {
    bit<32> y;
    const bit<32> arg = 32w16;
    c(Y(arg)) cinst;

    apply {
        y = 5;
        cinst.apply(x);
        cinst.apply(y);
    }
}

control dproto(out bit<32> x);
package top(dproto _d);

top(d()) main;
