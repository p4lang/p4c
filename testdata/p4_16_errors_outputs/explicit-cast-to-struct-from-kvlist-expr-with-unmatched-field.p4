void test<T>(in T val) {
}
struct S1<T1, T2> {
}

control c0() {
    apply {
        test((S1<bit<4>, int<6>>){T = 0,y = 0});
    }
}

