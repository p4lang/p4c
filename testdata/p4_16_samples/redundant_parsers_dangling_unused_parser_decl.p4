// The RemoveRedundantParsers pass removes applications of parsers which
// unconditionally transition to the accept state from the start state.
// For example:
// parser callee() {
//     state start {
//         transition accept;
//    }
// }
//
// parser caller() {
//     callee() subparser;
//     state start {
//         subparser.apply();  // <--- removed by RemoveRedundantParsers pass
//         transition accept;
//     }
// }
//
// After the application of 'subparser' is removed by RemoveRedundantParsers,
// subparser's unused declaration as well as its type's declaration remains.
// This test validates that these declarations get removed after the RemoveRedundantParsers
// pass. If they aren't removed, a Compiler Bug will occur in one of the inlining passes
// in some cases, and other unexpected behavior could occur elsewhere as well.

#include <core.p4>

header h_t {
    bit<16> f;
}

struct headers {
    h_t h1;
    h_t h2;
}

// This unused parser type declaration remains after RemoveRedundantParsers.
parser RedundantParser(inout headers hdr) {
    state start {
        transition accept;
    }
}

parser IParser2(packet_in packet, inout headers hdr) {
state start {
    packet.extract(hdr.h2);
    transition select(hdr.h2.f) {
      default: accept;
    }
  }
}

parser IParser(packet_in packet, inout headers hdr) {
  // This unused local parser declaration remains after RemoveRedundantParsers.
  RedundantParser() redundant_parser;

  state start {
    packet.extract(hdr.h1);
    transition select(hdr.h1.f) {
      1 : s1;
      default : s2;
    }
  }

  state s1 {
    // The below application of redundant_parser gets optimized out by RemoveRedundantParsers.
    redundant_parser.apply(hdr);
    transition accept;
  }

  state s2 {
    IParser2.apply(packet, hdr);
    transition accept;
  }
}

parser Parser(packet_in p, inout headers hdr);
package top(Parser p);
top(IParser()) main;
