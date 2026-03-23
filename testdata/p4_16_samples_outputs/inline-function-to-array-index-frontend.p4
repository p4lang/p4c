#include <core.p4>

header h_t {
}

struct Headers {
    h_t[1] hdr_arr;
}

control proto(inout Headers h);
package top(proto _c);
control ingress(inout Headers h) {
    @name("ingress.hdr") h_t hdr_0;
    @name("ingress.val_0") bit<3> val;
    @name("ingress.bound_0") bit<3> bound;
    @name("ingress.retval") bit<3> retval;
    @name("ingress.tmp") bit<3> tmp;
    @name("ingress.inlinedRetval") bit<3> inlinedRetval_1;
    @name("ingress.val_1") bit<3> val_2;
    @name("ingress.bound_1") bit<3> bound_2;
    @name("ingress.retval") bit<3> retval_1;
    @name("ingress.tmp") bit<3> tmp_1;
    @name("ingress.inlinedRetval_0") bit<3> inlinedRetval_2;
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @name("ingress.a") action a() {
        val = 3w0;
        bound = 3w0;
        if (val < bound) {
            tmp = val;
        } else {
            tmp = bound;
        }
        retval = tmp;
        inlinedRetval_1 = retval;
        hdr_0 = h.hdr_arr[inlinedRetval_1];
        val_2 = 3w0;
        bound_2 = 3w0;
        if (val_2 < bound_2) {
            tmp_1 = val_2;
        } else {
            tmp_1 = bound_2;
        }
        retval_1 = tmp_1;
        inlinedRetval_2 = retval_1;
        h.hdr_arr[inlinedRetval_2] = hdr_0;
    }
    @name("ingress.t") table t_0 {
        actions = {
            a();
            @defaultonly NoAction_1();
        }
        default_action = NoAction_1();
    }
    apply {
        t_0.apply();
    }
}

top(ingress()) main;
