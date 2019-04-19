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
header H1 { bit<32> f; }
header H2 { bit<32> g; }

header_union U {
    H1 h1;
    H2 h2;
}

control ct(out bit<32> b);
package top(ct _ct);

control c(out bit<32> x) {
    apply {
        U u;
        U[2] u2;

        bool b = u.isValid();
        b = b || u.h1.isValid();

        x = u.h1.f + u.h2.g;
        u.h1.setValid();
        u.h1.f = 0;
        x = x + u.h1.f;

        u.h2.g = 0;
        x = x + u.h2.g;

        u2[0].h1.setValid();
        u2[0].h1.f = 2;
        x = x + u2[1].h2.g + u2[0].h1.f;
    }
}
top(c()) main;