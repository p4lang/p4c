void tuple_array_bool_slice_loop(in bit<32> a, in bit<32> b, in bit<8> replacement, out bit<32> first, out bit<32> second, out bit<32> total) {
    tuple<bool, bit<3>, bit<12>, bit<16>>[2] entries;
    entries[0] = { a[0:0] == 1w1, a[3:1], a[15:4], a[31:16] };
    entries[1] = { b[0:0] == 1w1, b[3:1], b[15:4], b[31:16] };
    for (bit<32> i = 0; i < 2; i = i + 1) {
        if (entries[i][0]) {
            entries[i][2][7:0] = entries[i][2][7:0] + (bit<8>)entries[i][1];
            entries[i][3][11:4] = replacement;
        }
    }
    first = entries[0][3] ++ entries[0][2] ++ entries[0][1] ++ (bit<1>)entries[0][0];
    second = entries[1][3] ++ entries[1][2] ++ entries[1][1] ++ (bit<1>)entries[1][0];
    total = (bit<32>)entries[0][2] + (bit<32>)entries[1][2];
}
struct packet_t {
    bit<32> f1;
    bit<32> f2;
    bit<32> f3;
    bit<32> f4;
    bit<32> f5;
    bit<8>  x;
}

control c1(inout packet_t pkt) {
    action doit() {
        tuple_array_bool_slice_loop(pkt.f1, pkt.f2, pkt.x, pkt.f3, pkt.f4, pkt.f5);
    }
    apply {
        doit();
    }
}

control c<H>(inout H h);
package top<H>(c<H> c);
top(c1()) main;
