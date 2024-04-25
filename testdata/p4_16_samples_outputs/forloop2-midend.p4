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
    @name("c.hasReturned") bool hasReturned;
    @name("c.retval") bit<64> retval;
    @name("c.n") bit<64> n_0;
    @name("c.v") bit<64> v_0;
    @name("c.popcnti") bit<64> popcnti_0;
    @hidden action forloop2l18() {
        hasReturned = true;
        retval = n_0;
    }
    @hidden action forloop2l19() {
        n_0 = n_0 + 64w1;
        v_0 = v_0 & v_0 + 64w18446744073709551615;
    }
    @hidden action forloop2l14() {
        hasReturned = false;
        n_0 = 64w0;
        v_0 = hdrs.t1.v;
    }
    @hidden action forloop2l22() {
        retval = n_0;
    }
    @hidden action forloop2l27() {
        hdrs.t1.v = retval;
    }
    @hidden table tbl_forloop2l14 {
        actions = {
            forloop2l14();
        }
        const default_action = forloop2l14();
    }
    @hidden table tbl_forloop2l18 {
        actions = {
            forloop2l18();
        }
        const default_action = forloop2l18();
    }
    @hidden table tbl_forloop2l19 {
        actions = {
            forloop2l19();
        }
        const default_action = forloop2l19();
    }
    @hidden table tbl_forloop2l22 {
        actions = {
            forloop2l22();
        }
        const default_action = forloop2l22();
    }
    @hidden table tbl_forloop2l27 {
        actions = {
            forloop2l27();
        }
        const default_action = forloop2l27();
    }
    apply {
        tbl_forloop2l14.apply();
        for (popcnti_0 in 64w1 .. 64w63) {
            if (v_0 == 64w0) {
                tbl_forloop2l18.apply();
                break;
            }
            if (hasReturned) {
                ;
            } else {
                tbl_forloop2l19.apply();
            }
        }
        if (hasReturned) {
            ;
        } else {
            tbl_forloop2l22.apply();
        }
        tbl_forloop2l27.apply();
    }
}

top<headers_t>(c()) main;
