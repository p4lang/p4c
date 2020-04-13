#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

parser p() {
    bit<32> x_0;
    state start {
        transition select(x_0) {
            32w0: reject;
        }
    }
}

parser e();
package top(e e);
top(p()) main;

