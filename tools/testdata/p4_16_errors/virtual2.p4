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
extern Virtual {
    abstract bit<16> f(in bit<16> ix);
}

control c(inout bit<16> p) {
    Virtual() cntr = {
        void f(in bit<16> ix) {
            return (ix + 1);
        }
    };

    apply {
        p = cntr.f(6);
    }
}

control ctr(inout bit<16> x);
package top(ctr ctrl);

top(c()) main;
