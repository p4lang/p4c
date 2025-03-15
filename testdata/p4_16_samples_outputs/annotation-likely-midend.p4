#include <core.p4>

@command_line("--Wwarn=branch") struct Headers {
    bit<8> a;
    bit<8> b;
}

control ingress(inout Headers h) {
    @hidden action annotationlikely13() {
        h.a = 8w0;
        h.b = 8w0;
    }
    @hidden table tbl_annotationlikely13 {
        actions = {
            annotationlikely13();
        }
        const default_action = annotationlikely13();
    }
    apply {
        tbl_annotationlikely13.apply();
    }
}

control c<T>(inout T d);
package top<T>(c<T> _c);
top<Headers>(ingress()) main;
