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

#include <core.p4>

parser Prs<T>(packet_in b, out T result);
control Map<T>(in T d);

package Switch<T>(Prs<T> prs, Map<T> map);

parser P(packet_in b, out bit<32> d) { state start { transition accept; } }
control Map1(in bit<32> d) { apply {} }
control Map2(in bit<8> d) { apply {} }

Switch(P(),
       Map1()) main;

Switch<bit<32>>(P(),
                Map1()) main1;
