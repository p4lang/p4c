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

control p(out bit y)
{
    action a(in bit x0, out bit y0)
    {
        bit x = x0;
        y0 = x0 & x;
    }

    action b(in bit x, out bit y)
    {
        bit z;
        a(x, z);
        a(z & z, y);
    }

    apply {
        bit x = 1;
        b(x, y);
    }
}

control simple(out bit y);
package m(simple pipe);

m(p()) main;
