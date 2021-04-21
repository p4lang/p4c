#include <core.p4>

header ethernet_t {
    bit<48> src_addr;
}

struct Headers {
    ethernet_t eth_hdr;
}

control ingress(inout Headers h) {
    @name("ingress.tmp") bit<16> tmp;
    @name("ingress.tmp_bool") bool tmp_bool_0;
    @name("ingress.val_undefined") bool val_undefined;
    @name("ingress.tmp") bit<16> tmp_3;
    @name("val_undefined") bool val_undefined_1;
    @name(".do_global_action") action do_global_action() {
        tmp = 16w0;
    }
    @noWarn("unused") @name(".NoAction") action NoAction_0() {
    }
    @name("ingress.do_action") action do_action() {
        tmp_3 = tmp_3 * (tmp_bool_0 ? 16w0 : 16w1);
        tmp_bool_0 = val_undefined;
    }
    @name("ingress.simple_table") table simple_table_0 {
        key = {
            h.eth_hdr.src_addr: exact @name("h.eth_hdr.src_addr") ;
        }
        actions = {
            do_action();
            do_global_action();
            @defaultonly NoAction_0();
        }
        default_action = NoAction_0();
    }
    @hidden action issue2393l18() {
        tmp_bool_0 = false;
    }
    @hidden table tbl_issue2393l18 {
        actions = {
            issue2393l18();
        }
        const default_action = issue2393l18();
    }
    apply {
        tbl_issue2393l18.apply();
        simple_table_0.apply();
    }
}

control c<H>(inout H h);
package top<H>(c<H> _c);
top<Headers>(ingress()) main;

