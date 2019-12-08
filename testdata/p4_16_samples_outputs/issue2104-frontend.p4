#include <core.p4>
#include <v1model.p4>

control c() {
    @name("c.v") action v() {
        bool hasReturned = false;
        hasReturned = true;
    }
    apply {
        v();
    }
}

control e();
package top(e e);
top(c()) main;

