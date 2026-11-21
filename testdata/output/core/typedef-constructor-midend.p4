extern MyCounter<I> {
    MyCounter(bit<32> size);
    void count(in I index);
}

control Test() {
    @name("Test.counter_set_1") MyCounter<bit<10>>(32w1024) counter_set;
    @name("Test.counter_set_2") MyCounter<bit<10>>(32w1024) counter_set_0;
    @hidden action typedefconstructor17() {
        counter_set.count(10w42);
        counter_set_0.count(10w42);
    }
    @hidden table tbl_typedefconstructor17 {
        actions = {
            typedefconstructor17();
        }
        const default_action = typedefconstructor17();
    }
    apply {
        tbl_typedefconstructor17.apply();
    }
}

control C();
package P(C _c);
P(Test()) main;
