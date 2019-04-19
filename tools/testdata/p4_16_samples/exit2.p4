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

control ctrl(out bit<32> c) {
    bit<32> x;

    action e() {
        exit;
        x = 1;
    }

    apply {
        bit<32> a;
        bit<32> b;

        a = 0;
        b = 1;
        c = 2;
        if (a == 0) {
            b = 2;
            e();
            c = 3;
        } else {
            b = 3;
            e();
            c = 4;
        }
        c = 5;
    }
}

control noop(out bit<32> c);
package p(noop _n);
p(ctrl()) main;
