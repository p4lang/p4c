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
    @name("ingress.tmp_idx") bit<3> tmp_idx_0;
    @name("ingress.val_0") bit<3> val;
    @name("ingress.bound_0") bit<3> bound;
    @name("ingress.retval") bit<3> retval;
    @name("ingress.tmp") bit<3> tmp;
    @name("ingress.inlinedRetval") bit<3> inlinedRetval_0;
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
        inlinedRetval_0 = retval;
        tmp_idx_0 = inlinedRetval_0;
        hdr_0 = h.hdr_arr[tmp_idx_0];
        h.hdr_arr[tmp_idx_0] = hdr_0;
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
