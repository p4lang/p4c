control Test(out bit<8> first, out bit<8> second, out bit<8> counter) {
    @name("Test.call_with_side_effect") action call_with_side_effect() {
        counter = 8w0;
        counter = 8w1;
        first = 8w1;
        second = 8w1;
    }
    @hidden table tbl_call_with_side_effect {
        actions = {
            call_with_side_effect();
        }
        const default_action = call_with_side_effect();
    }
    apply {
        tbl_call_with_side_effect.apply();
    }
}

control Pipeline(out bit<8> first, out bit<8> second, out bit<8> counter);
package Top(Pipeline pipeline);
Top(Test()) main;
