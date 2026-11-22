control C();
package P(C c);
control MyC() {
    bit<8> x;
    apply {
        x = 8w1;
    }
}

P(MyC()) main;
