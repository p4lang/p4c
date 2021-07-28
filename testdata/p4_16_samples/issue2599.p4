extern void __semaphore_write(in bit<32> data = 0);

control c() {
    apply {
        __semaphore_write();
        __semaphore_write();
    }
}


control proto();
package top(proto p);

top(c()) main;
