control Test(out bit<8> first, out bit<8> second, out bit<8> counter) {
    @name("Test.tmp") bit<8> tmp;
    @name("Test.value") bit<8> value;
    @name("Test.value_1") bit<8> value_2;
    @name("Test.retval") bit<8> retval;
    @name("Test.inlinedRetval") bit<8> inlinedRetval_0;
    @name("Test.call_with_side_effect") action call_with_side_effect() {
        counter = 8w0;
        value_2 = counter;
        value_2 = value_2 + 8w1;
        retval = value_2;
        counter = value_2;
        inlinedRetval_0 = retval;
        tmp = inlinedRetval_0;
        value = tmp;
        first = value;
        second = value;
    }
    apply {
        call_with_side_effect();
    }
}

control Pipeline(out bit<8> first, out bit<8> second, out bit<8> counter);
package Top(Pipeline pipeline);
Top(Test()) main;
