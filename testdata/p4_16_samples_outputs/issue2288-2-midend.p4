struct Headers {
    bit<8> a;
    bit<8> b;
}

control ingress(inout Headers h) {
    @hidden action issue22882l23() {
        h.a = 8w3;
        h.a = 8w3;
    }
    @hidden table tbl_issue22882l23 {
        actions = {
            issue22882l23();
        }
        const default_action = issue22882l23();
    }
    apply {
        tbl_issue22882l23.apply();
    }
}

control c<T>(inout T d);
package top<T>(c<T> _c);
top<Headers>(ingress()) main;
