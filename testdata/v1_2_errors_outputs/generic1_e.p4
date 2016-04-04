extern If<T> {
    T id(in T d);
}

control p1(in If x) {
    apply {
    }
}

control p2(in If<int<32>, int<32>> x) {
    apply {
    }
}

header h {
}

control p() {
    apply {
        h<bit<1>> x;
    }
}

