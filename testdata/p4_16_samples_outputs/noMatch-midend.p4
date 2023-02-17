#include <core.p4>

parser p() {
    @name("p.x") bit<32> x_0;
    state noMatch {
        verify(false, error.NoMatch);
        transition reject;
    }
    state start {
        transition select(x_0) {
            32w0: reject;
            default: noMatch;
        }
    }
}

parser e();
package top(e e);
top(p()) main;
