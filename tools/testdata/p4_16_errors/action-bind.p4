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
control c(inout bit<32> x) {
    bit<32> y;
    action a(inout bit<32> b, bit<32> d) {
        b = d;
    }
    table t {
        actions = { a(x); }
        default_action = a(y, 0);  // error: different bound argument
    }
    apply {
        t.apply();
    }
}

control proto(inout bit<32> x);
package top(proto p);

top(c()) main;
