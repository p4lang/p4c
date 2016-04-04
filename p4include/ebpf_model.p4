#ifndef _EBPF_MODEL_P4_
#define _EBPF_MODEL_P4_

#include "core.p4"

const Version ebpf_version = { 8w0, 8w1 };

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
