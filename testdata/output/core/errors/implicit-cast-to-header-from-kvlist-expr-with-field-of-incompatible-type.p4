header H {
    varbit<32> f;
}

void f(out H h) {
    h = {f = 10};
}
