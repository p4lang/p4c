#include <core.p4>

extern Register<T, I> {
    Register(bit<32> size, T initial_value);
}

extern RegisterAction<T, I, U> {
    RegisterAction(Register<_, _> reg);
    abstract void apply(inout T value, @optional out U rv);
}

parser p() {
    @name("p.reg") Register<bit<128>, bit<16>>(32w0, 128w0) reg_0;
    @name("p.regAct") RegisterAction<bit<128>, bit<16>, bit<64>>(reg_0) regAct_0 = {
        void apply(inout bit<128> val, out bit<64> rv) {
            return;
        }
    };
    state start {
        transition accept;
    }
}

parser simple();
package top(simple s);
top(p()) main;
