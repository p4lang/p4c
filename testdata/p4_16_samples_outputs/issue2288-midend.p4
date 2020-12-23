#include <core.p4>

struct Headers {
    bit<8> a;
    bit<8> b;
}

control ingress(inout Headers h) {
    @name("ingress.tmp_0") bool tmp_0;
    @name("ingress.tmp_1") bit<8> tmp_1;
    @name("ingress.tmp_3") bit<8> tmp_3;
    @name("ingress.a") action a_1() {
        h.b = 8w0;
    }
    @name("ingress.t") table t_0 {
        key = {
            h.b: exact @name("h.b") ;
        }
        actions = {
            a_1();
        }
        default_action = a_1();
    }
    @hidden action act() {
        tmp_0 = true;
    }
    @hidden action act_0() {
        tmp_0 = false;
    }
    @hidden action issue2288l25() {
        tmp_1 = h.a;
    }
    @hidden action issue2288l25_0() {
        tmp_1 = h.b;
    }
    @hidden action act_1() {
        tmp_3 = h.a;
        h.a = 8w3;
        h.a = tmp_3;
    }
    @hidden table tbl_act {
        actions = {
            act();
        }
        const default_action = act();
    }
    @hidden table tbl_act_0 {
        actions = {
            act_0();
        }
        const default_action = act_0();
    }
    @hidden table tbl_issue2288l25 {
        actions = {
            issue2288l25();
        }
        const default_action = issue2288l25();
    }
    @hidden table tbl_issue2288l25_0 {
        actions = {
            issue2288l25_0();
        }
        const default_action = issue2288l25_0();
    }
    @hidden table tbl_act_1 {
        actions = {
            act_1();
        }
        const default_action = act_1();
    }
    apply {
        if (t_0.apply().hit) {
            tbl_act.apply();
        } else {
            tbl_act_0.apply();
        }
        if (tmp_0) {
            tbl_issue2288l25.apply();
        } else {
            tbl_issue2288l25_0.apply();
        }
        tbl_act_1.apply();
    }
}

control c<T>(inout T d);
package top<T>(c<T> _c);
top<Headers>(ingress()) main;

