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

extern X {
    X();
    bit<32> b();
    abstract void a(inout bit<32> arg);
}

control c(inout bit<32> y) {
    X() x = {
        void a(inout bit<32> arg) { arg = arg + this.b(); }
    };
    apply {
        x.a(y);
    }
}

control t(inout bit<32> b) {
    c() c1;
    c() c2;

    apply {
        c1.apply(b);
        c2.apply(b);
    }
}

control cs(inout bit<32> arg);
package top(cs _ctrl);

top(t()) main;
