struct packet_t {
    bit<32> f1;
    bit<32> f2;
    bit<32> f3;
    bit<32> f4;
    bit<32> f5;
    bit<8>  x;
}

struct tuple_0 {
    bool    f0;
    bit<3>  f1;
    bit<12> f2;
    bit<16> f3;
}

control c1(inout packet_t pkt) {
    @name("c1.entries") tuple_0[2] entries_0;
    @name("c1.doit") action doit() {
        entries_0[0].f0 = pkt.f1[0:0] == 1w1;
        entries_0[0].f1 = pkt.f1[3:1];
        entries_0[0].f2 = pkt.f1[15:4];
        entries_0[0].f3 = pkt.f1[31:16];
        entries_0[1].f0 = pkt.f2[0:0] == 1w1;
        entries_0[1].f1 = pkt.f2[3:1];
        entries_0[1].f2 = pkt.f2[15:4];
        entries_0[1].f3 = pkt.f2[31:16];
        if (pkt.f1[0:0] == 1w1) {
            entries_0[32w0].f2[7:0] = pkt.f1[11:4] + (bit<8>)pkt.f1[3:1];
            entries_0[32w0].f3[11:4] = pkt.x;
        }
        if (pkt.f2[0:0] == 1w1) {
            entries_0[32w1].f2[7:0] = pkt.f2[11:4] + (bit<8>)pkt.f2[3:1];
            entries_0[32w1].f3[11:4] = pkt.x;
        }
        pkt.f3 = entries_0[0].f3 ++ entries_0[0].f2 ++ pkt.f1[3:1] ++ (bit<1>)(pkt.f1[0:0] == 1w1);
        pkt.f4 = entries_0[1].f3 ++ entries_0[1].f2 ++ pkt.f2[3:1] ++ (bit<1>)(pkt.f2[0:0] == 1w1);
        pkt.f5 = (bit<32>)entries_0[0].f2 + (bit<32>)entries_0[1].f2;
    }
    @hidden table tbl_doit {
        actions = {
            doit();
        }
        const default_action = doit();
    }
    apply {
        tbl_doit.apply();
    }
}

control c<H>(inout H h);
package top<H>(c<H> c);
top<packet_t>(c1()) main;
