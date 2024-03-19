control C();
package P(C c);
control MyC() {
    bit<8> x;
    apply {
        x = 1;
    }
}

P(MyC()) main;
