#include <core.p4>
#include <v1model.p4>

parser p() {
    bit<32> x;
    state start {
        transition select(x) {
            32w0: reject;
        }
    }
}

parser e();
package top(e e);
top(p()) main;

