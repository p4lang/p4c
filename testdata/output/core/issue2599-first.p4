extern void __semaphore_write(in bit<32> data=0);
control c() {
    apply {
        __semaphore_write(data = 32w0);
        __semaphore_write(data = 32w0);
    }
}

control proto();
package top(proto p);
top(c()) main;
