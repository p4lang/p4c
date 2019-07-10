struct PortId_t {
    bit<9> _v;
}

const PortId_t PSA_CPU_PORT = { 9w192 };
struct metadata_t {
    PortId_t foo;
}

control I(inout metadata_t meta) {
    apply {
        if (meta.foo == { 9w192 }) {
            meta.foo._v = meta.foo._v + 9w1;
        }
    }
}

control C<M>(inout M m);
package top<M>(C<M> c);
top<metadata_t>(I()) main;

