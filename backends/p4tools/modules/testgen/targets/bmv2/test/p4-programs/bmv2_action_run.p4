#include <v1model.p4>

header ethernet_t {
    bit<48> dst_addr;
    bit<48> src_addr;
    bit<16> eth_type;
}

header H {
    bit<8> a;
    bit<8> a1;
    bit<8> b;
}

struct Headers {
    ethernet_t eth_hdr;
    H          h;
}

struct Meta {}

parser p(packet_in pkt, out Headers h, inout Meta meta, inout standard_metadata_t stdmeta) {
    state start {
        pkt.extract(h.eth_hdr);
        transition parse_h;
    }
    state parse_h {
        pkt.extract(h.h);
        transition accept;
    }
}

control vrfy(inout Headers h, inout Meta meta) {
    apply { }
}

control ingress(inout Headers h, inout Meta m, inout standard_metadata_t s) {

    action MyAction1() {
        h.h.b = 1;
    }

    action MyAction2() {
        h.h.b = 2;
    }
    
    action MyAction3(){
        h.h.a1 = 1;
    }

    action MyAction4(){
        h.h.a1 = 2;
    }
    
    action MyAction5(){
        h.h.a1 = 3;
    }
  
    table exact_table {
        key = {
            h.h.a : exact;
        }

        actions = {
            NoAction;
            MyAction1;
            MyAction2;
        }

        const entries = {
            8w1 : MyAction1();
            8w2 : MyAction2();
        }

        size = 1024;
        default_action = NoAction();
    }

    apply {
        switch (exact_table.apply().action_run) {
          MyAction1: {MyAction3();}
          MyAction2: {MyAction4();}
          NoAction: {MyAction5();}
        }
    }
}

control egress(inout Headers h, inout Meta m, inout standard_metadata_t s) {
    apply { }
}

control update(inout Headers h, inout Meta m) {
    apply { }
}

control deparser(packet_out pkt, in Headers h) {
    apply {
        pkt.emit(h);
    }
}

V1Switch(p(), vrfy(), ingress(), egress(), update(), deparser()) main;