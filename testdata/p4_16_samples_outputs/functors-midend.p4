#include <core.p4>

parser nothing();
package m(nothing n);
parser p_0() {
    state start {
        transition accept;
    }
}

m(p_0()) main;

