extern MyCounter<I> {
    MyCounter(bit<32> size);
    void count(in I index);
}

typedef bit<10> my_counter_index_t;
typedef MyCounter<my_counter_index_t> my_counter_t;
control Test() {
    @name("Test.counter_set_1") MyCounter<my_counter_index_t>(32w1024) counter_set;
    @name("Test.counter_set_2") my_counter_t(32w1024) counter_set_0;
    apply {
        counter_set.count(10w42);
        counter_set_0.count(10w42);
    }
}

control C();
package P(C _c);
P(Test()) main;
