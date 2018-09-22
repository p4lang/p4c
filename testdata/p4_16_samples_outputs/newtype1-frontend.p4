typedef bit<9> Narrow_t;
type Narrow_t Narrow;
typedef bit<32> Wide_t;
type Wide_t Wide;
control c(out bool b) {
    Wide x;
    Narrow y;
    apply {
        x = (Wide)32w3;
        y = (Narrow)(Narrow_t)(Wide_t)x;
        b = y == (Narrow)9w10;
    }
}

control ctrl(out bool b);
package top(ctrl _c);
top(c()) main;

