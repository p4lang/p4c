#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

const bool test = static_assert(20180101 >= 20160101, "V1 model version is not >= 20160101");
void f() {
    static_assert(static_assert(true, ""));
}
control c() {
    apply {
        f();
    }
}

control ct();
package pt(ct c);
pt(c()) main;
