extern MyCounter<I> {
    MyCounter(bit<32> size);
    void count(in I index);
}

typedef bit<10> my_counter_index_t;
typedef MyCounter<my_counter_index_t> my_counter_t;

control Test() {
    // This compiles:
    MyCounter<my_counter_index_t>(1024) counter_set_1;

    // And this should also compile:
    my_counter_t(1024) counter_set_2;

    apply {
        counter_set_1.count(42);
        counter_set_2.count(42);
    }
}

control C();
package P(C _c);
P(Test()) main;
