#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

control c() {
    bit<16> F = 16w0;
    bit<128> Y = 128w0;
    action r() {
        Y = (bit<128>)F;
    }
    action v() {
        return;
        r();
        F = (bit<16>)Y;
    }
    apply {
        v();
    }
}

control e();
package top(e e);
top(c()) main;
