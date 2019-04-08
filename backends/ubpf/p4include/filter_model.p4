#ifndef _FILTER_MODEL_P4_
#define _FILTER_MODEL_P4_

#include <core.p4>

/**
 Implementation property for tables indicating that tables must be implemented
 using EBPF hash map.
*/
extern hash_table {
    /// @param size: maximum number of entries in table
    hash_table(bit<32> size);
}

parser parse<H>(packet_in packet, out H headers);
control filter<H>(inout H headers, out bool accept);

package Filter<H>(parse<H> prs,
                      filter<H> filt);

#endif

