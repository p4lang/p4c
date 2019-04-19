/*
Copyright 2018 Barefoot Networks, Inc. 

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

extern ext<H, V> {
    ext(H v);
    V method1<T>(in H h, in T t);
    H method2<T>(in T t);
}

control c() {
    ext<bit<16>, void>(16w0) x;
    apply {
        x.method1(x.method2(12w1), 8w0);
    }
}
