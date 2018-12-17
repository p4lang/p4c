#include <core.p4>

parser p() {
    state start {
        transition reject;
    }
}

parser s();
package top(s _s);
top(p()) main;

