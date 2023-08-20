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
    
    action MyAction3(bool table_hit){
      if (table_hit) {
        h.h.a1 = 8w1;
      } else {
        h.h.a1 = 8w2;
      }
    }
  
    action MyAction4() {
        h.h.b = 4;
    }

    action MyAction5() {
        h.h.b = 5;
    }

    action MyAction6() {
        h.h.b = 6;
    }
    
    action MyAction7() {
        h.h.b = 7;
    }
    
    action MyAction8() {
        h.h.b = 8;
    }

    action MyAction9() {
        h.h.b = 9;
    }

    action MyAction10() {
        h.h.b = 10;
    }

    action MyAction11() {
        h.h.b = 11;
    }

    action MyAction12() {
        h.h.b = 12;
    }

    action MyAction13() {
        h.h.b = 13;
    }

    action MyAction14() {
        h.h.b = 14;
    }

    action MyAction15() {
        h.h.b = 15;
    }

    action MyAction16() {
        h.h.b = 16;
    }

    action MyAction17() {
        h.h.b = 17;
    }

    action MyAction18() {
        h.h.b = 18;
    }

    action MyAction19() {
        h.h.b = 19;
    }

    action MyAction20() {
        h.h.b = 20;
    }

    table hit_table {
        key = {
            h.h.a : exact;
        }

        actions = {
            NoAction;
            MyAction1;
            MyAction2;
            MyAction4;
            MyAction5;
            MyAction6;
            MyAction7;
            MyAction8;
            MyAction9;
            MyAction10;
            MyAction11;
            MyAction12;
            MyAction13;
            MyAction14;
            MyAction15;
            MyAction16;
            MyAction17;
            MyAction18;
            MyAction19;
            MyAction20;
        }

        const entries = {
            8w1 : MyAction1();
            8w2 : MyAction2();
            8w4 : MyAction4();
            8w5 : MyAction5();
            8w6 : MyAction6();
            8w7 : MyAction7();
            8w8 : MyAction8();
            8w9 : MyAction9();
            8w10 : MyAction10();
            8w11 : MyAction11();
            8w12 : MyAction12();
            8w13 : MyAction13();
            8w14 : MyAction14();
            8w15 : MyAction15();
            8w16 : MyAction16();
            8w17 : MyAction17();
            8w18 : MyAction18();
            8w19 : MyAction19();
            8w20 : MyAction20();
        }

        size = 1024;
        default_action = NoAction();
    }

    apply {
        if(hit_table.apply().hit) {
            MyAction3(true);
        } else {
            MyAction3(false);
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