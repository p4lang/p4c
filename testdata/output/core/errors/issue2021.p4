control C();
package S(C c);
const bit<8> x = 8w1;
extern void f(out bit<8> a);
control MyC() {
    apply {
        .x = 8w3;
        f(.x);
    }
}

S(MyC()) main;
