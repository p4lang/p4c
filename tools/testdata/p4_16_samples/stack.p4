/*
Copyright 2013-present Barefoot Networks, Inc.

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

header h {}

parser p()
{
    state start {
        h[4] stack;

        stack[3].setValid();
        h b = stack[3];
        b = stack.last;
        stack[2] = b;
        b = stack.next;
        bit<32> e = stack.lastIndex;
        transition accept;
    }
}

control c() {
    apply {
        h[4] stack;
        stack[3].setValid();
        h b = stack[3];
        stack[2] = b;
        stack.push_front(2);
        stack.pop_front(2);
        bit<32> sz = stack.size;
    }
}

parser Simple();
control Simpler();
package top(Simple par, Simpler ctr);
top(p(), c()) main;
