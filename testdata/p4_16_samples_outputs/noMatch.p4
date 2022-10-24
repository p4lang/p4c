#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

parser p() {
    state start {
        bit<32> x;
        transition select(x) {
            0: reject;
        }
    }
}

parser e();
package top(e e);
top(p()) main;
