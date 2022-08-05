#include <core.p4>

header ethernet_t {
    bit<48> src_addr;
}

struct Headers {
    ethernet_t eth_hdr;
}

control ingress(inout Headers h) {
    @name("ingress.tmp") bit<16> tmp;
    @name("ingress.filler_bool") bool filler_bool_0;
    @name("ingress.tmp_bool") bool tmp_bool_0;
    @name("ingress.tmp") bool tmp_0;
    @name("ingress.make_zero") bool make_zero_0;
    @name("ingress.val_undefined") bool val_undefined_0;
    @name("ingress.tmp") bit<16> tmp_4;
    @name("ingress.make_zero") bool make_zero_2;
    @name("ingress.val_undefined") bool val_undefined_2;
    @name(".do_global_action") action do_global_action_0() {
        make_zero_2 = true;
        tmp = tmp * (make_zero_2 ? 16w0 : 16w1);
        filler_bool_0 = val_undefined_2;
    }
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @name("ingress.do_action") action do_action() {
        tmp_0 = tmp_bool_0;
        make_zero_0 = tmp_0;
        tmp_4 = tmp_4 * (make_zero_0 ? 16w0 : 16w1);
        tmp_bool_0 = val_undefined_0;
    }
    @name("ingress.simple_table") table simple_table_0 {
        key = {
            h.eth_hdr.src_addr: exact @name("h.eth_hdr.src_addr") ;
        }
        actions = {
            do_action();
            do_global_action_0();
            @defaultonly NoAction_1();
        }
        const default_action = NoAction_1();
    }
    apply {
        tmp_bool_0 = false;
        simple_table_0.apply();
    }
}

control c<H>(inout H h);
package top<H>(c<H> _c);
top<Headers>(ingress()) main;

