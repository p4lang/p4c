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

extern bit<32> f(in bit<32> x);

control c(inout bit<32> r) {
    apply {
        if ((f(2) > 0 && f(3) < 0) || f(5) == 2) {
            r = 1;
        } else {
            r = 2;
        }
    }
}

control simple(inout bit<32> r);
package top(simple e);
top(c()) main;
