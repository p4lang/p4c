/*
Copyright 2016-present Barefoot Networks, Inc.

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

// issue #113
#include<core.p4>
#include<v1model.p4>

control C(bit<1> meta) {
    apply {
        if ((meta & 0x0) == 0) {
            digest(0, meta); // this lines causes trouble
        }
    }
}