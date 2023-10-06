parser MyParser<t>(t tt) {
    state start {
        transition accept;
    }
}

parser MyParser1() {
    state start {
        MyParser<bit>.apply(1);
        transition accept;
    }
}
