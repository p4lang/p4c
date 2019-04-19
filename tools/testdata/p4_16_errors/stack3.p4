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

/* This test illustrates several limitations and incorrect uses of
 * header stacks, which must be declared with compile-time constant
 * sizes. */

#include <core.p4>

header h {}

parser p() {
    state start {
        transition accept;
    }
}

control c()(bit<32> x) {
    apply {
        h[4] s1;
        h[s1.size + 1] s2;
        h[x] s3;
   }
}

parser Simple();
control Simpler();
package top(Simple par, Simpler ctr);
top(p(), c(32w1)) main;
