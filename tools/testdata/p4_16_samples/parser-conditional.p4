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

#include <core.p4>

parser p(out bit<32> b) {
    bit<32> a = 1;
    state start {
        b = (a == 0) ? 32w2 : 3;
        b = b + 1;
        b = (a > 0) ? ((a > 1) ? b+1 : b+2) : b+3;
        transition accept;
    }
}

parser proto(out bit<32> b);
package top(proto _p);

top(p()) main;
