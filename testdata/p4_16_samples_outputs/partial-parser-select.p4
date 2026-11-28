#include <core.p4>

enum Mode {
    A,
    B
}

typedef Mode Alias;
enum bit<8> Protocol {
    A = 1,
    B = 2,
    C = 3
}

typedef Protocol ProtocolAlias;
parser P(in bit<8> x)(tuple<bit<8>, bit<8>> key) {
    state start {
        transition select(Mode.A, x) {
            (Alias.B, default): reject;
            (Alias.A, 7): nested;
            default: ranges;
        }
    }
    state nested {
        transition select({ 8w1, 8w2 }, x) {
            (key, 7): ranges;
            default: reject;
        }
    }
    state ranges {
        transition select(ProtocolAlias.B, x) {
            (Protocol.A .. ProtocolAlias.C, 7): masks;
            (Protocol.C .. Protocol.C, default): reject;
            default: no_match;
        }
    }
    state masks {
        transition select(ProtocolAlias.B, x) {
            (Protocol.B &&& ProtocolAlias.C, 7): accept;
            (Protocol.C &&& Protocol.C, default): reject;
            default: no_match;
        }
    }
    state no_match {
        transition select(8w1, x) {
            (2, 7): accept;
        }
    }
}

parser PT(in bit<8> x);
package Top(PT p);
Top(P({ 8w1, 8w2 })) main;
