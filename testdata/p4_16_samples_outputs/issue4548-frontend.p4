#include <core.p4>

control ingress() {
    apply {
    }
}

control Ingress();
package top(Ingress ig);
top(ingress()) main;
