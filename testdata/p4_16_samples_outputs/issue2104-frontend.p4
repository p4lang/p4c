#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

control c() {
    @name("c.v") action v() {
    }
    apply {
        v();
    }
}

control e();
package top(e e);
top(c()) main;
