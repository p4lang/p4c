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

#include <core.p4>
#include <v1model.p4>

control empty();
package top(empty e);
control Ing() {
    bool b;
    bit<32> a;
    bool tmp_1;
    bool tmp_2;
    @name("cond") action cond_0() {
        b = true;
        tmp_1 = (!true && !true ? false : tmp_1);
        tmp_2 = (!true && !!true ? a == 32w5 : tmp_2);
        tmp_1 = (!true && !!true ? (!true && !!true ? a == 32w5 : tmp_2) : (!true && !true ? false : tmp_1));
    }
    table tbl_cond {
        actions = {
            cond_0();
        }
        const default_action = cond_0();
    }
    apply {
        tbl_cond.apply();
    }
}

top(Ing()) main;
