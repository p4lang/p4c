#include <core.p4>

struct headers {
    bit<32> x;
}

extern void f(headers h);
control c() {
    apply {
        headers h;
        f({ 5 });
    }
}

control C();
package top(C _c);
top(c()) main;

