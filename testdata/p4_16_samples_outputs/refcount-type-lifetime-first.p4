extern bool ready();
extern bit<8> identity(in bit<8> value);
extern void consume(in bit<8> value);
control C() {
    bit<8> value = 8w1;
    apply {
        if (ready()) {
            value = identity(value);
            consume(value);
        }
    }
}

control CT();
package Test(CT c);
Test(C()) main;
