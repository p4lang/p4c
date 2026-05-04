/*
 * SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
 * Copyright 2013-present Barefoot Networks, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
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
    /** Add value to counter with specified index. */
    void add(in bit<32> index, in bit<32> value);
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
