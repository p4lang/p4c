#include <core.p4>

header inner_t {
    bit<8> f2;
}

header outer_t {
    bit<8> f1;
}

struct mid_t {
    inner_t[1] inner_arr;
}

struct Headers {
    outer_t[1] outer_arr;
    mid_t[1]   mid_arr;
}

control proto(inout Headers h);
package top(proto _c);
control ingress(inout Headers h) {
    @name("ingress.val") bit<8> val;
    @name("ingress.tmp_idx") bit<3> tmp_idx_1;
    @name("ingress.tmp_idx_0") bit<3> tmp_idx_2;
    @name("ingress.val_0") bit<3> val_3;
    @name("ingress.bound_0") bit<3> bound;
    @name("ingress.retval") bit<3> retval;
    @name("ingress.tmp") bit<3> tmp;
    @name("ingress.inlinedRetval") bit<3> inlinedRetval_1;
    @name("ingress.val_2") bit<3> val_4;
    @name("ingress.bound_1") bit<3> bound_2;
    @name("ingress.retval") bit<3> retval_1;
    @name("ingress.tmp") bit<3> tmp_1;
    @name("ingress.inlinedRetval_0") bit<3> inlinedRetval_2;
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @name("ingress.a") action a() {
        val_3 = 3w0;
        bound = 3w0;
        if (val_3 < bound) {
            tmp = val_3;
        } else {
            tmp = bound;
        }
        retval = tmp;
        inlinedRetval_1 = retval;
        tmp_idx_1 = inlinedRetval_1;
        val_4 = 3w0;
        bound_2 = 3w0;
        if (val_4 < bound_2) {
            tmp_1 = val_4;
        } else {
            tmp_1 = bound_2;
        }
        retval_1 = tmp_1;
        inlinedRetval_2 = retval_1;
        tmp_idx_2 = inlinedRetval_2;
        val = 8w1;
        h.mid_arr[tmp_idx_1].inner_arr[tmp_idx_2].f2 = val;
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
