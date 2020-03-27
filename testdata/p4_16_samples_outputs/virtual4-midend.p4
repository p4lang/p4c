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
    @name(".NoAction") action NoAction_0() {
    }
    @name(".NoAction") action NoAction_3() {
    }
    @name("ingress.stack") Stack<bit<16>>(2048) stack_0;
    @name("ingress.write") StackAction<bit<16>, bit<16>>(stack_0) write_0 = {
        void apply(inout bit<16> value) {
        }
        void overflow(inout bit<16> value, out bit<16> rv) {
        }
    };
    @name("ingress.read") StackAction<bit<16>, bit<16>>(stack_0) read_0 = {
        void apply(inout bit<16> value, out bit<16> rv) {
        }
        void underflow(inout bit<16> value, out bit<16> rv) {
        }
    };
    @name("ingress.push") action push_1() {
        hdr.data.b1 = 8w0xff;
        write_0.push();
    }
    @name("ingress.do_push") table do_push_0 {
        actions = {
            push_1();
            @defaultonly NoAction_0();
        }
        key = {
            hdr.data.f1: ternary @name("hdr.data.f1") ;
        }
        default_action = NoAction_0();
    }
    @name("ingress.pop") action pop_1() {
        hdr.data.b1 = 8w0xfe;
        hdr.data.h1 = read_0.pop();
    }
    @name("ingress.do_pop") table do_pop_0 {
        actions = {
            pop_1();
            @defaultonly NoAction_3();
        }
        key = {
            hdr.data.f1: exact @name("hdr.data.f1") ;
        }
        default_action = NoAction_3();
    }
    apply {
        if (hdr.data.b1 == 8w0) {
            do_pop_0.apply();
        } else {
            do_push_0.apply();
        }
    }
}

control ctr<H>(inout H hdr);
package top<H>(ctr<H> ctrl);
top<headers>(ingress()) main;

