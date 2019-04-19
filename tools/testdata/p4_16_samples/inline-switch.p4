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

control c(out bit<32> x) {
    action a1() {}
    action a2() {}
    action a3() {}

    table t {
        actions = { a1; a2; }
        default_action = a1;
    }

    apply {
        switch (t.apply().action_run) {
            a1:
            a2: { return; }
            default: { return; }
        }
    }
}

control d(out bit<32> x) {
    c() cinst;

    apply {
        cinst.apply(x);
    }
}

control dproto(out bit<32> x);
package top(dproto _d);

top(d()) main;
