#include <core.p4>

extern test_extern<T> {
    test_extern();
    void write(in T value);
}

test_extern<test_extern<bit<32>>>() test;

extern test_extern1<T> {
    test_extern1();
    void write(in T value);
}

test_extern1<test_extern<test_extern1<bit<32>>>>() test1;

