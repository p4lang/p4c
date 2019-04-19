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

#include <core.p4>

control Ing(out bit<32> a) {
    bool b;

    action cond() {
        if (b)  // <<< use of b
           a = 5;
        else
           a = 10;
    }

    apply {
        b = true;  // <<< incorrectly removed: issue #210
        cond();
    }
}

control s(out bit<32> a);
package top(s e);

top(Ing()) main;
