parser MyParser<t>(t tt) {
    state start {
        transition accept;
    }
}

parser MyParser1() {
    state start {
        MyParser<bit<1>>.apply(1);
        transition accept;
    }
}

