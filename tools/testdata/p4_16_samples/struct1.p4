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

struct P
{
    bit<32> f1;
    bit<32> f2;
}

struct T
{
    int<32> t1;
    int<32> t2;
}

struct S
{
    T s1;
    T s2;
}

const T t = { 10, 20 };
const S s = { { 15, 25 }, t };

const int<32> x = t.t1;
const int<32> y = s.s1.t2;

const int<32> w = .t.t1;

const T t1 = s.s1;
