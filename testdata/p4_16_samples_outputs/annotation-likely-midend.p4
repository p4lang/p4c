#include <core.p4>

struct Headers {
    bit<8> a;
    bit<8> b;
}

control ingress(inout Headers h) {
    @hidden action annotationlikely11() {
        h.a = 8w0;
        h.b = 8w0;
    }
    @hidden table tbl_annotationlikely11 {
        actions = {
            annotationlikely11();
        }
        const default_action = annotationlikely11();
    }
    apply {
        tbl_annotationlikely11.apply();
    }
}

control c<T>(inout T d);
package top<T>(c<T> _c);
top<Headers>(ingress()) main;
