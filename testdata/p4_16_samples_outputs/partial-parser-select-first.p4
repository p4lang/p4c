#include <core.p4>

enum Mode {
    A,
    B
}

typedef Mode Alias;
enum bit<8> Protocol {
    A = 8w1,
    B = 8w2,
    C = 8w3
}

typedef Protocol ProtocolAlias;
parser P(in bit<8> x)(tuple<bit<8>, bit<8>> key) {
    state start {
        transition select(x) {
            8w7: nested;
            default: ranges;
        }
    }
    state nested {
        transition select({ 8w1, 8w2 }, x) {
            (key, 8w7): ranges;
            default: reject;
        }
    }
    state ranges {
        transition select(x) {
            8w7: masks;
            default: no_match;
        }
    }
    state masks {
        transition select(x) {
            8w7: accept;
            default: no_match;
        }
    }
    state no_match {
        transition select(x) {
        }
    }
}

parser PT(in bit<8> x);
package Top(PT p);
Top(P({ 8w1, 8w2 })) main;
