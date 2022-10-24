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
    @name("ingress.stack") Stack<bit<16>>(2048) stack_0;
    @name("ingress.read") StackAction<bit<16>, bit<16>>(stack_0) read_0 = {
        void underflow(inout bit<16> value, out bit<16> rv) {
            rv = 16w0xf0f0;
        }
    };
    @hidden action issue2273l48() {
        hdr.data.h1 = read_0.pop();
    }
    @hidden table tbl_issue2273l48 {
        actions = {
            issue2273l48();
        }
        const default_action = issue2273l48();
    }
    apply {
        tbl_issue2273l48.apply();
    }
}

control ctr<H>(inout H hdr);
package top<H>(ctr<H> ctrl);
top<headers>(ingress()) main;
