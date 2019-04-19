#include <core.p4>

control C1();
control C2();
control C3();
extern E {
    E();
    void set1(C1 c1);
    void set2(C2 c2);
    C1 get1();
    C2 get2();
}

package S(C3 c3);
control MyC1()(E e) {
    apply {
        C2 c2 = e.get2();
        c2.apply();
    }
}

control MyC2()(E e) {
    apply {
        C1 c1 = e.get1();
        c1.apply();
    }
}

control MyC3()(E e, C1 c1, C2 c2) {
    apply {
        e.set1(c1);
        e.set2(c2);
        c1.apply();
    }
}

E() e;

S(MyC3(e, MyC1(e), MyC2(e))) main;

