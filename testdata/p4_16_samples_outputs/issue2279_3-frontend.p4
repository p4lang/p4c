header ethernet_t {
    bit<48> dst_addr;
    bit<48> src_addr;
    bit<16> eth_type;
}

struct Headers {
    ethernet_t eth_hdr;
}

control c(inout Headers hdr) {
    @name("c.tmp_val") bool tmp_val_0;
    @name("c.do_action") action do_action() {
        hdr.eth_hdr.eth_type = 16w3 + (tmp_val_0 ? 16w3 : 16w1);
        tmp_val_0 = true;
    }
    @name("c.do_action") action do_action_1() {
        hdr.eth_hdr.eth_type = 16w3 + (tmp_val_0 ? 16w3 : 16w1);
        tmp_val_0 = true;
    }
    apply {
        tmp_val_0 = false;
        do_action();
        do_action_1();
    }
}

control proto(inout Headers hdr);
package top(proto _p);
top(c()) main;

