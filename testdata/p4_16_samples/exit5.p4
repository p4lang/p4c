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

control ctrl() {
    action e() {
        exit;
    }
    action f() {}

    table t {
        actions = { e; f; }
        default_action = e();
    }

    apply {
        bit<32> a;
        bit<32> b;
        bit<32> c;

        a = 0;
        b = 1;
        c = 2;
        switch (t.apply().action_run) {
            e: {
                b = 2;
                t.apply();
                c = 3;
            }
            f: {
                b = 3;
                t.apply();
                c = 4;
            }
        }
        c = 5;
    }
}

control noop();
package p(noop _n);
p(ctrl()) main;
