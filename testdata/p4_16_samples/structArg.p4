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
struct S {
    bit<32> f;
}

control c(inout S data) {
    apply {
        data.f = 0;
    }
}

control caller() {
    S data;
    c() cinst;

    apply {
        data.f = 0;
        cinst.apply(data);
    }
}

control none();
package top(none n);

top(caller()) main;
