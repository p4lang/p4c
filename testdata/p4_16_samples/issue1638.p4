#include <core.p4>

struct intrinsic_metadata_t {
   bit<8> f0;
   bit<8> f1;
}

struct empty_t {}

control C<H, M>(
    inout H hdr,
    inout M meta,
    in intrinsic_metadata_t intr_md);

package P<H, M>(C<H, M> c);

struct hdr_t { }
struct meta_t { bit<8> f0; bit<8> f1; bit<8> f2; }

control MyC2(in meta_t meta = {0, 0, 0}) {
  table a {
    key = {
      meta.f0 : exact;
    }
    actions = { NoAction; }
  }
  apply {
    a.apply();
  }
}

control MyC(inout hdr_t hdr, inout meta_t meta, in intrinsic_metadata_t intr_md) {
   MyC2() c2;
   apply {
     c2.apply();
   }
}

P(MyC()) main;
