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

#include <core.p4>

control c(out bool x) {
    table t1 {
        key = { x : exact; }
        actions = { NoAction; }
        default_action = NoAction();
    }
    table t2 {
        key = { x : exact; }
        actions = { NoAction; }
        default_action = NoAction();
    }
    apply {
        x = true;
        if (t1.apply().hit && t2.apply().hit)
            x = false;
    }
}

control proto(out bool x);
package top(proto p);

top(c()) main;
