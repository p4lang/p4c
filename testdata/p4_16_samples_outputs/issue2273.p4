#include <core.p4>

extern Stack<T> {
    Stack(int size);
}

extern StackAction<T, U> {
    StackAction(Stack<T> stack);
    U pop();
    @synchronous(pop) @optional abstract void underflow(inout T value, out U rv);
}

header data_t {
    bit<16> h1;
}

struct headers {
    data_t data;
}

control ingress(inout headers hdr) {
    Stack<bit<16>>(2048) stack;
    StackAction<bit<16>, bit<16>>(stack) read = {
        void underflow(inout bit<16> value, out bit<16> rv) {
            rv = 0xf0f0;
        }
    };
    apply {
        hdr.data.h1 = read.pop();
    }
}

control ctr<H>(inout H hdr);
package top<H>(ctr<H> ctrl);
top(ingress()) main;
