#include <core.p4>

int<8> mul(in int<8> a, int<8> b=2, in int<8> c) {
    return a;
}
int<8> mul(in int<8> a, in int<8> b, in int<8> c=3) {
    return a;
}
control MyControl(out int<8> output) {
    apply {
        int<8> x = mul(2, 3);
        output = x;
    }
}

control Control(out int<8> output);
package SimplePipeline(Control c);
SimplePipeline(MyControl()) main;
