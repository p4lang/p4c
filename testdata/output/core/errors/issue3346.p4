parser MyParser1(in bit<6> v) {
    value_set<int>(4) myvs;
    state start {
        transition select(v) {
            myvs: accept;
            default: reject;
        }
    }
}

