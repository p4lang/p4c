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

header H { bit z; }

extern bit<32> f(inout bit x, in bit b);

control c(out H[2] h);
package top(c _c);

control my(out H[2] s) {
    apply {
        bit<32> a = 0;
        s[a].z = 1;
        s[a+1].z = 0;
        a = f(s[a].z, 0);
        a = f(s[a].z, 1);
    }
}

top(my()) main;
