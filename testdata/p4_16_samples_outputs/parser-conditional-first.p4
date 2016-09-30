parser p() {
    bit<32> a = 32w1;
    bit<32> b;
    state start {
        b = (a == 32w0 ? 32w2 : 32w3);
        b = b + 32w1;
        b = (a > 32w0 ? (a > 32w1 ? b + 32w1 : b + 32w2) : b + 32w3);
        transition accept;
    }
}

parser proto();
package top(proto _p);
top(p()) main;
