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

control p()
{
    action b(in bit<1> x, out bit<1> y)
    {
        bit<1> z;

        {
            bit<1> x_0;
            x_0 = x;
            z = (x & x_0);
            y = z;
        }
    }

    apply {
        bit<1> x_1 = 0;
        bit<1> y_0;
        b(x_1, y_0);
    }
}

control simple();
package m(simple pipe);

.m(.p()) main;
