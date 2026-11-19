struct packet_t {
    bit<32> f1;
    bit<32> f2;
    bit<32> f3;
    bit<32> f4;
    bit<32> f5;
    bit<8>  x;
}

control c1(inout packet_t pkt) {
    @name("c1.a_0") bit<32> a;
    @name("c1.b_0") bit<32> b;
    @name("c1.replacement_0") bit<8> replacement;
    @name("c1.first_0") bit<32> first;
    @name("c1.second_0") bit<32> second;
    @name("c1.total_0") bit<32> total;
    @name("c1.entries") tuple<bool, bit<3>, bit<12>, bit<16>>[2] entries_0;
    @name("c1.i") bit<32> i_0;
    @name("c1.doit") action doit() {
        a = pkt.f1;
        b = pkt.f2;
        replacement = pkt.x;
        entries_0[0] = { a[0:0] == 1w1, a[3:1], a[15:4], a[31:16] };
        entries_0[1] = { b[0:0] == 1w1, b[3:1], b[15:4], b[31:16] };
        for (i_0 = 32w0; i_0 < 32w2; i_0 = i_0 + 32w1) {
            if (entries_0[i_0][0]) {
                entries_0[i_0][2][7:0] = entries_0[i_0][2][7:0] + (bit<8>)entries_0[i_0][1];
                entries_0[i_0][3][11:4] = replacement;
            }
        }
        first = entries_0[0][3] ++ entries_0[0][2] ++ entries_0[0][1] ++ (bit<1>)entries_0[0][0];
        second = entries_0[1][3] ++ entries_0[1][2] ++ entries_0[1][1] ++ (bit<1>)entries_0[1][0];
        total = (bit<32>)entries_0[0][2] + (bit<32>)entries_0[1][2];
        pkt.f3 = first;
        pkt.f4 = second;
        pkt.f5 = total;
    }
    apply {
        doit();
    }
}

control c<H>(inout H h);
package top<H>(c<H> c);
top<packet_t>(c1()) main;
