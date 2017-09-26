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
#include <v1model.p4>

typedef bit<16> Hash;

control p();
package top(p _p);

control c() {
    apply {
        bit<16> var;
        bit<32> hdr = 0;

        hash(var, HashAlgorithm.crc16, 16w0, hdr, 0xFFFF);
    }
}

top(c()) main;
