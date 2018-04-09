#include <core.p4>

control C();
package S(C c);
extern WrapControls<T1, T2> {
    WrapControls(T1 t1, T2 t2);
    void execute1();
    void execute2();
}

control MyC1(WrapControls wc) {
    apply {
        wc.execute2();
    }
}

control MyC2(WrapControls wc) {
    apply {
        wc.execute1();
    }
}

control MyC3() {
    MyC1() c1;
    MyC2() c2;
    WrapControls<MyC1, MyC2>(c1, c2) wc;
    apply {
        c1.apply(wc);
    }
}

S(MyC3()) main;

