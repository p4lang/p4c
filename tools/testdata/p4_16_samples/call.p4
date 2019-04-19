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

control qp()
{
    action drop() {}

    table m {
        actions = {
            drop;
        }
        default_action = drop;
    }

    apply {
        m.apply();
    }
}

extern Ix
{
    Ix();
    void f();
    void f1(in int<32> x);
    void g();
    int<32> h();
}

control p()
{
    Ix() x;
    Ix() y;

    action b(in bit<32> arg2)
    {}

    apply {
        x.f();
        x.f1(32s1);
        b(32w0);
    }
}
