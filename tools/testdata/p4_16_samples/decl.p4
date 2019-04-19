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

control p(in bit y_0)
{
    apply {
        bit x;
        x = 1w0;
    
        if (x == 1w0)
        {
            bit y;
            y = 1w1;
        }
        else
        {
            bit y;
            y = y_0;
            {
                bit y;
                y = 1w0;

                {
                    bit y;
                    y = 1w0;
                }
            }
        }
    }
}
