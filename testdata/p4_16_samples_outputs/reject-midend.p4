#include <core.p4>

parser f() {
    state start {
        transition reject;
    }
}

parser nothing();
package switch0(nothing _p);
switch0(f()) main;

