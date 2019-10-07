#ifndef _FILTER_MODEL_P4_
#define _FILTER_MODEL_P4_

#include <core.p4>

parser parse<H, M>(packet_in packet, out H headers, inout M meta);
control filter<H, M>(inout H headers, inout M meta);

package ubpf<H, M>(parse<H, M> prs,
                      filter<H, M> filt);

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

