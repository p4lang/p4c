bit<8> increment(inout bit<8> value) {
    value = value + 1;
    return value;
}

control Test(out bit<8> first, out bit<8> second, out bit<8> counter) {
    action use_twice(bit<8> value) {
        first = value;
        second = value;
    }

    action call_with_side_effect() {
        counter = 0;
        use_twice(increment(counter));
    }

    apply {
        call_with_side_effect();
    }
}

control Pipeline(out bit<8> first, out bit<8> second, out bit<8> counter);
package Top(Pipeline pipeline);

Top(Test()) main;
