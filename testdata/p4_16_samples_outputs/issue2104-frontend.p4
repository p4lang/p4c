#include <core.p4>

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
