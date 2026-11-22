header ethernet_t {
    bit<48> dst_addr;
    bit<48> src_addr;
    bit<16> eth_type;
}

struct Headers {
    ethernet_t eth_hdr;
}

control c(inout Headers hdr) {
    @name("c.do_action") action do_action() {
    }
    @name("c.do_action") action do_action_1() {
        hdr.eth_hdr.eth_type = 16w6;
    }
    @hidden table tbl_do_action {
        actions = {
            do_action();
        }
        const default_action = do_action();
    }
    @hidden table tbl_do_action_0 {
        actions = {
            do_action_1();
        }
        const default_action = do_action_1();
    }
    apply {
        tbl_do_action.apply();
        tbl_do_action_0.apply();
    }
}

control proto(inout Headers hdr);
package top(proto _p);
top(c()) main;
