extern MyCounter<I> {
    MyCounter(bit<32> size);
    void count(in I index);
}

control C();
control c() {
    MyCounter<int>(1024) counter;
    apply {
        counter.count(1024);
    }
}

package P(C c);
P(c()) main;
