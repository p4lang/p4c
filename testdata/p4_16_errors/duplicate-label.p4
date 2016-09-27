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

control c(out bit arun) {
    action a() {}
    table t {
        actions = { a; }
        default_action = a;
    }
    apply {
        switch (t.apply().action_run) {
            a: { arun = 1; }
            a: { arun = 1; }  // duplicate label
        }
    }
}

control proto(out bit run);
package top(proto _p);

top(c()) main;
