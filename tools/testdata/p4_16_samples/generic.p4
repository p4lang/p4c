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

extern Crc16<T> 
{}

extern Checksum16 
{
    void initialize<T>();
}

extern void f<T>(in T dt);

control q<S>(in S dt)
{
    apply {
        f<bit<32>>(32w0);
        f(32w0);
        f<S>(dt);
    }
}

extern X<D>
{
   T f<T>(in D d, in T t);
}

control z<D1, T1>(in X<D1> x, in D1 v, in T1 t)
{
   // x's type is X<D1>
   // x.f's type is T f<T>(D1 d, T t);

    apply {
        T1 r1 = x.f<T1>(v, t);
        T1 r2 = x.f(v, t);
    }
}
