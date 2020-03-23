#include <core.p4>
#define ISSUE 1

header data {
  bit<8> da;
  bit<8> db;
}

struct headers {
    data hdr;
}

struct metadata {
  bit<32> foo;
}

parser ParserImpl(
    packet_in b,
    out headers p,
    inout metadata m)
{
  state start {
    b.extract(p.hdr);
    m.foo = 32w1;
    transition select(p.hdr.da) {
        0xaa: parse_b;
#ifdef ISSUE
        default: reject;
#else
        default: accept;
#endif
    }
  }

  state parse_b {
      m.foo = 32w2;
      transition accept;
  }
}

parser P<H, M>(packet_in b, out H h, inout M m);
package top<H, M>(P<H, M> p);
top(ParserImpl()) main;