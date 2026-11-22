#include <core.p4>

extern MyCounter<I> {
    MyCounter(bit<32> size);
    void count(in I index);
}

control Test() {
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @name("Test.counter_set") MyCounter<bit<10>>(32w1024) counter_set_0;
    @name("Test.Inner.count") action Inner_count_0(@name("index") bit<10> index_1) {
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
