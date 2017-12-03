#include <core.p4>

parser f() {
    state start {
    }
}

parser nothing();
package switch0(nothing _p);
switch0(f()) main;

