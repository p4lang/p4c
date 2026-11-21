#include <core.p4>

parser PT(in bit<8> x);
package Top(PT p);
parser P_0(in bit<8> x) {
    state start {
        transition select(x) {
            8w7: nested;
            default: ranges;
        }
    }
    state nested {
        transition select(x) {
            8w7: ranges;
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

Top(P_0()) main;
