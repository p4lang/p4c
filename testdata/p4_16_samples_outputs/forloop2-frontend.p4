#include <core.p4>

control generic<M>(inout M m);
package top<M>(generic<M> c);
header t1 {
    bit<64> v;
}

struct headers_t {
    t1 t1;
}

control c(inout headers_t hdrs) {
    @name("c.val_0") bit<64> val;
    @name("c.hasReturned") bool hasReturned;
    @name("c.retval") bit<64> retval;
    @name("c.n") bit<64> n_0;
    @name("c.v") bit<64> v_0;
    @name("c.popcnti") bit<64> popcnti_0;
    @name("c.inlinedRetval") bit<64> inlinedRetval_0;
    apply {
        val = hdrs.t1.v;
        hasReturned = false;
        n_0 = 64w0;
        v_0 = val;
        for (@name("c.popcnti") bit<64> popcnti_0 in 64w1 .. 64w63) {
            if (v_0 == 64w0) {
                hasReturned = true;
                retval = n_0;
                break;
            } else {
                n_0 = n_0 + 64w1;
                v_0 = v_0 & v_0 + 64w18446744073709551615;
            }
        }
        if (hasReturned) {
            ;
        } else {
            retval = n_0;
        }
        inlinedRetval_0 = retval;
        hdrs.t1.v = inlinedRetval_0;
    }
}

top<headers_t>(c()) main;
