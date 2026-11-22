#include <core.p4>

extern MyCounter<I> {
    MyCounter(bit<32> size);
    void count(in I index);
}

typedef bit<10> my_counter_index_t;
typedef MyCounter<my_counter_index_t> my_counter_t;
control Test() {
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @name("Test.counter_set") my_counter_t(32w1024) counter_set_0;
    @name("Test.Inner.count") action Inner_count_0(@name("index") my_counter_index_t index_1) {
        counter_set_0.count(index_1);
    }
    @name("Test.Inner.counter_table") table Inner_counter_table {
        actions = {
            Inner_count_0();
            @defaultonly NoAction_1();
        }
        default_action = NoAction_1();
    }
    apply {
        Inner_counter_table.apply();
    }
}

control C();
package P(C _c);
P(Test()) main;
