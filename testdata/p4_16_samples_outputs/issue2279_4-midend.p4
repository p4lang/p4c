header ethernet_t {
    bit<48> dst_addr;
    bit<48> src_addr;
    bit<16> eth_type;
}

struct Headers {
    ethernet_t eth_hdr;
}

control c(inout Headers hdr) {
    @name("c.tmp_val") bit<16> tmp_val_0;
    @name("c.do_action") action do_action() {
        hdr.eth_hdr.eth_type = 16w6;
    }
    @name("c.do_action") action do_action_1() {
        hdr.eth_hdr.eth_type = 16w3 + (tmp_val_0 > 16w2 ? 16w3 : 16w1);
    }
    @hidden action issue2279_4l12() {
        tmp_val_0 = 16w3;
    }
    @hidden action issue2279_4l19() {
        tmp_val_0 = (hdr.eth_hdr.eth_type >> 1) + 16w65533;
    }
    @hidden table tbl_issue2279_4l12 {
        actions = {
            issue2279_4l12();
        }
        const default_action = issue2279_4l12();
    }
    @hidden table tbl_do_action {
        actions = {
            do_action();
        }
        const default_action = do_action();
    }
    @hidden table tbl_issue2279_4l19 {
        actions = {
            issue2279_4l19();
        }
        const default_action = issue2279_4l19();
    }
    @hidden table tbl_do_action_0 {
        actions = {
            do_action_1();
        }
        const default_action = do_action_1();
    }
    apply {
        tbl_issue2279_4l12.apply();
        tbl_do_action.apply();
        tbl_issue2279_4l19.apply();
        tbl_do_action_0.apply();
    }
}

control proto(inout Headers hdr);
package top(proto _p);
top(c()) main;
