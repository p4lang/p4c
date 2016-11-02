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

#ifndef _EBPF_MODEL_P4_
#define _EBPF_MODEL_P4_

#include <core.p4>

extern CounterArray {
    /* Allocate an array of counters.
     * @param size    Maximum counter index supported.
     * @param sparse  The counter array is supposed to be sparse. */
    CounterArray(bit<32> max_index, bool sparse);
    /* Increment counter with specified index. */
    void increment(in bit<32> index);
}

extern array_table {
    array_table(bit<32> size);
}

extern hash_table {
    hash_table(bit<32> size);
}

/* architectural model for EBPF packet filter target architecture */

parser parse<H>(packet_in packet, out H headers);
control filter<H>(inout H headers, out bool accept);

package ebpfFilter<H>(parse<H> prs,
                      filter<H> filt);

#endif
