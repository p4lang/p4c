#include <core.p4>

control Wrapper(inout bit<8> value) {
    @name("Wrapper.wrapped.set") action wrapped_set_0(@name("value") bit<8> value_1) {
        value = value_1;
    }
    @name("Wrapper.wrapped.doSet") table wrapped_doSet {
        actions = {
            wrapped_set_0();
        }
        default_action = wrapped_set_0(8w0);
    }
    apply {
        wrapped_doSet.apply();
    }
}

control c(inout bit<8> v);
package top(c _c);
top(Wrapper()) main;
