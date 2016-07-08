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
#include "core.p4"

header H {
    bit<32> field;
}

parser P(packet_in p, out H h) {
    state start {
        p.extract(h);
        transition next;
    }

    state next {
        p.extract(h);  // error: overwriting header
    }
}

parser Simple<T>(packet_in p, out T t);
package top<T>(Simple<T> prs);
top(P()) main;
