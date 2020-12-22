struct Headers {
    bit<8> a;
}

control ingress(inout Headers h) {
    @name("ingress.tmp") bit<8> tmp;
    @hidden action act() {
        tmp = h.a;
        h.a = 8w3;
        h.a = tmp;
    }
    @hidden table tbl_act {
        actions = {
            act();
        }
        const default_action = act();
    }
    apply {
        tbl_act.apply();
    }
}

control c<T>(inout T d);
package top<T>(c<T> _c);
top<Headers>(ingress()) main;

