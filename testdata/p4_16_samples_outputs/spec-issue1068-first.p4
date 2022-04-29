parser MyParser<t>(t tt) {
    state start {
        transition accept;
    }
}

parser MyParser1() {
    @name("MyParser") MyParser<bit<1>>() MyParser_inst;
    state start {
        MyParser_inst.apply(1w1);
        transition accept;
    }
}

