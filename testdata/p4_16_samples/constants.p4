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
    apply {
        bit x;
        bit z;
        const bit c = 1w0;
        
        if (true)
            x = 1w0;
        else
            x = 1w1;
        
        if (false)
            z = 1w0;
        else if (true && false)
            z = 1w1;    
        
        if (c == 1w0)
            z = 1w0;
    }
}
