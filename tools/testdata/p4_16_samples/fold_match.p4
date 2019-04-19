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

parser p()
{
    state start
    {
        transition select(32w0)
        {
            32w5 &&& 32w7 : reject;
            32w0          : accept;
            default       : reject;
        }
    }

    state next0
    {
        transition select(32w0)
        {
            32w1 &&& 32w1 : reject;
            default       : accept;
            32w0          : reject;
        }
    }

    state next1
    {
        transition select(32w1)
        {
            32w1 &&& 32w1 : accept;
            32w0          : reject;
            default       : reject;
        }
    }

    state next2
    {
        transition select(32w1)
        {
            32w0 .. 32w7 : accept;
            32w0         : reject;
            default       : reject;
        }
    }

    state next3
    {
        transition select(32w3)
        {
            32w1 &&& 32w1 : accept;
            32w0          : reject;
            default       : reject;
        }
    }

    state next00
    {
        transition select(true, 32w0)
        {
            (true, 32w1 &&& 32w1) : reject;
            default       : accept;
            (true, 32w0)          : reject;
        }
    }

    state next01
    {
        transition select(true, 32w1)
        {
            (true, 32w1 &&& 32w1) : accept;
            (true, 32w0)          : reject;
            default       : reject;
        }
    }

    state next02
    {
        transition select(true, 32w1)
        {
            (true, 32w0 .. 32w7) : accept;
            (true, 32w0)          : reject;
            default       : reject;
        }
    }

    state next03
    {
        transition select(true, 32w3)
        {
            (true, 32w1 &&& 32w1) : accept;
            (true, 32w0)          : reject;
            default       : reject;
        }
    }

    state last
    {
        transition select(32w0)
        {
            32w5 &&& 32w7 : reject;
            32w1          : reject;
        }
    }
}
