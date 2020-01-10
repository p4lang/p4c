/*
Copyright 2019 Orange

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

#ifndef _FILTER_MODEL_P4_
#define _FILTER_MODEL_P4_

#include <core.p4>

parser parse<H, M>(packet_in packet, out H headers, inout M meta);
control filter<H, M>(inout H headers, inout M meta);
@deparser
control deparser<H>(packet_out b, in H headers);

package ubpf<H, M>(parse<H, M> prs,
                filter<H, M> filt,
                deparser<H> dprs);

extern void mark_to_drop();
extern void mark_to_pass();

extern Register<T, S> {
  Register(bit<32> size);

  T read  (in S index);
  void write (in S index, in T value);
}

extern bit<48> ubpf_time_get_ns();

enum HashAlgorithm {
    lookup3
}

extern void hash<D>(out bit<32> result, in HashAlgorithm algo, in D data);

#endif

