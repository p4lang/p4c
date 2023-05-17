#include <core.p4>

header ethernet_t {
    bit<48> src_addr;
}

struct Headers {
    ethernet_t eth_hdr;
}

action do_global_action(in bool make_zero, out bool val_undefined) {
    bit<16> tmp;
    tmp = tmp *  (make_zero ? 16w0: 16w1);
}

control ingress(inout Headers h) {
    bool filler_bool = true;
    bool tmp_bool = false;
    action do_action() {
        do_global_action(tmp_bool, tmp_bool);
    }

    table simple_table {
        key = {
            h.eth_hdr.src_addr : exact;
        }
        actions = {
            do_action();
            do_global_action(true, filler_bool);
        }
    }
    apply {
        simple_table.apply();
    }
}

control c<H>(inout H h);
package top<H>(c<H> _c);

top(ingress()) main;
