typedef bit<9> Narrow_t;
type Narrow_t Narrow;
typedef bit<32> Wide_t;
type Wide_t Wide;
control c(out bool b) {
    Wide x_0;
    Narrow y_0;
    apply {
        x_0 = (Wide)32w3;
        y_0 = (Narrow)(Narrow_t)(Wide_t)x_0;
        b = y_0 == (Narrow)9w10;
    }
}

control ctrl(out bool b);
package top(ctrl _c);
top(c()) main;

