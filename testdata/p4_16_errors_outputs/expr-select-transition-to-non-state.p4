parser MyParser10() {
    value_set<bit<6>>(4) myvs;
    state start {
        transition select(6w1) {
            myvs: myvs;
        }
    }
}

