#include <core.p4>

control Wrapped(inout bit<8> valueSet) {
    @name("set") action set(bit<8> value) {
        valueSet = value;
    }
    @name("doSet") table doSet {
        actions = {
            set();
        }
        default_action = set(8w0);
    }
   apply {
        doSet.apply();
   }
}

control Wrapper(inout bit<8> value) {
   Wrapped() wrapped;
   apply { wrapped.apply(value); }
}

control c(inout bit<8> v);
package top(c _c);

top(Wrapper()) main;
