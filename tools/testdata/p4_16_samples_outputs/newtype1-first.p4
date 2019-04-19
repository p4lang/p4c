typedef bit<9> Narrow_t;
type Narrow_t Narrow;
typedef bit<32> Wide_t;
type Wide_t Wide;
const Narrow PSA_CPU_PORT = (Narrow)9w192;
control c(out bool b) {
    apply {
        Wide x = (Wide)32w3;
        Narrow y = (Narrow)(Narrow_t)(Wide_t)x;
        b = y == (Narrow)9w10;
    }
}

control ctrl(out bool b);
package top(ctrl _c);
top(c()) main;

