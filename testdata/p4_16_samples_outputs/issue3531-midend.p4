#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

control c() {
    apply {
    }
}

control ct();
package pt(ct c);
pt(c()) main;

