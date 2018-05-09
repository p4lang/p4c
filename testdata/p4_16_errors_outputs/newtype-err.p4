typedef bit<32> B32;
type bit<32> N32;
control c(out B32 x) {
    apply {
        B32 b = 0;
        N32 n;
        N32 n1;
        n = b + b;
        n1 = n + 0;
        x = n;
    }
}

control e(out B32 x);
package top(e _e);
top(c()) main;

