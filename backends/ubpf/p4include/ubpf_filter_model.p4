#ifndef _FILTER_MODEL_P4_
#define _FILTER_MODEL_P4_

#include <core.p4>

parser parse<H>(packet_in packet, out H headers);
control filter<H>(inout H headers);

package ubpfFilter<H>(parse<H> prs,
                      filter<H> filt);

extern void mark_to_drop();

extern Register<T, S> {
  Register(bit<32> size);

  T read  (in S index);
  void write (in S index, in T value);
}

extern bit<48> ubpf_time_get_ns();

#endif

