#include <core.p4>

extern Stack<T> {
    Stack(int size);
}

extern StackAction<T, U> {
    StackAction(Stack<T> stack);
    U push();
    U pop();
    @synchronous(push, pop) abstract void apply(inout T value, @optional out U rv);
    @synchronous(push) @optional abstract void overflow(@optional inout T value, @optional out U rv);
    @synchronous(pop) @optional abstract void underflow(@optional inout T value, @optional out U rv);
}

header data_t {
    bit<32> f1;
    bit<32> f2;
    bit<16> h1;
    bit<8>  b1;
    bit<8>  b2;
}

struct headers {
    data_t data;
}

control ingress(inout headers hdr) {
    Stack<bit<16>>(2048) stack;
    StackAction<bit<16>, bit<16>>(stack) write = {
        void apply(inout bit<16> value) {
            value = hdr.data.h1;
        }
        void overflow(inout bit<16> value, out bit<16> rv) {
            rv = 0xf0f;
        }
    };
    StackAction<bit<16>, bit<16>>(stack) read = {
        void apply(inout bit<16> value, out bit<16> rv) {
            rv = value;
        }
        void underflow(inout bit<16> value, out bit<16> rv) {
            rv = 0xf0f0;
        }
    };
    action push() {
        hdr.data.b1 = 0xff;
        write.push();
    }
    table do_push {
        actions = {
            push;
        }
        key = {
            hdr.data.f1: ternary;
        }
    }
    action pop() {
        hdr.data.b1 = 0xfe;
        hdr.data.h1 = read.pop();
    }
    table do_pop {
        actions = {
            pop;
        }
        key = {
            hdr.data.f1: exact;
        }
    }
    apply {
        if (hdr.data.b1 == 0) {
            do_pop.apply();
        } else {
            do_push.apply();
        }
    }
}

control ctr<H>(inout H hdr);
package top<H>(ctr<H> ctrl);
top(ingress()) main;
