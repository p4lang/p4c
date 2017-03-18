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

parser p() {
    state start {
        bit<8> x = 5;
        transition select(x, x, {x, x}, x) {
            (0, 0, {0, 0}, 0): accept;
            (1, 1, default, 1): accept;
            default: reject;
        }
    }
}

parser s();
package top(s _s);

top(p()) main;
