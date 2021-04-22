#include <core.p4>

header ethernet_t {
    bit<48> src_addr;
}

struct Headers {
    ethernet_t eth_hdr;
}

control ingress(inout Headers h) {
    @name(".do_global_action") action do_global_action(@name("make_zero") in bool make_zero_1, @name("val_undefined") out bool val_undefined_1) {
        @name("ingress.tmp") bit<16> tmp;
        tmp = tmp * (make_zero_1 ? 16w0 : 16w1);
    }
    @noWarn("unused") @name(".NoAction") action NoAction_0() {
    }
    @name("ingress.filler_bool") bool filler_bool_0;
    @name("ingress.tmp_bool") bool tmp_bool_0;
    @name("ingress.tmp") bool tmp_0;
    @name("ingress.do_action") action do_action() {
        tmp_0 = tmp_bool_0;
        {
            @name("ingress.make_zero") bool make_zero = tmp_0;
            @name("ingress.val_undefined") bool val_undefined;
            @name("ingress.tmp") bit<16> tmp_3;
            tmp_3 = tmp_3 * (make_zero ? 16w0 : 16w1);
            tmp_bool_0 = val_undefined;
        }
    }
    @name("ingress.simple_table") table simple_table_0 {
        key = {
            h.eth_hdr.src_addr: exact @name("h.eth_hdr.src_addr") ;
        }
        actions = {
            do_action();
            do_global_action(true, filler_bool_0);
            @defaultonly NoAction_0();
        }
        default_action = NoAction_0();
    }
    apply {
        tmp_bool_0 = false;
        simple_table_0.apply();
    }
}

control c<H>(inout H h);
package top<H>(c<H> _c);
top<Headers>(ingress()) main;

