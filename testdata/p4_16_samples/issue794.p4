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

control c();
parser p();
package Top(c i, p prs);

extern Random2 {
    Random2();
    bit<10> read();
}

parser callee(Random2 rand) {
    state start {
        rand.read();
        transition accept;
    }
}

parser caller() {
    Random2() rand1;
    callee() ca;

    state start {
        ca.apply(rand1);
        transition accept;
    }
}

control foo2(Random2 rand) {
    apply {
        rand.read();
    }
}

control ingress() {
    Random2() rand1;
    foo2() foo2_inst;

    apply {
        foo2_inst.apply(rand1);
    }
}


Top(ingress(), caller()) main;
