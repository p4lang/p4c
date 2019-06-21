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

/**
   A counter array is a dense or sparse array of unsigned 32-bit values, visible to the
   control-plane as an EBPF map (array or hash).
   Each counter is addressed by a 32-bit index.
   Counters can only be incremented by the data-plane, but they can be read or
   reset by the control-plane.
 */
extern CounterArray {
    /** Allocate an array of counters.
     * @param max_index  Maximum counter index supported.
     * @param sparse     The counter array is supposed to be sparse. */
    CounterArray(bit<32> max_index, bool sparse);
    /** Increment counter with specified index. */
    void increment(in bit<32> index);
}

/*
 Each table must have an implementation property which is either an array_table
 or a hash_table.
*/

/**
 Implementation property for tables indicating that tables must be implemented
 using EBPF array map.  However, if a table uses an LPM match type, the implementation
 is only used for the size, and the table used is an LPM trie.
*/
extern array_table {
    /// @param size: maximum number of entries in table
    array_table(bit<32> size);
}

/**
 Implementation property for tables indicating that tables must be implemented
 using EBPF hash map.
*/
extern hash_table {
    /// @param size: maximum number of entries in table
    hash_table(bit<32> size);
}

/* architectural model for EBPF packet filter target architecture */

parser parse<H>(packet_in packet, out H headers);
control filter<H>(inout H headers, out bool accept);

package ebpfFilter<H>(parse<H> prs,
                      filter<H> filt);

#endif
