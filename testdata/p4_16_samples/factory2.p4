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
#include <core.p4>

extern widget<T> { }

extern widget<T> createWidget<T, U>(U a, T b);

parser P();
parser p1()(widget<bit<8>> w) {
    state start { transition accept; } }

package sw0(P p);

sw0(p1(createWidget(16w0, 8w0))) main;
