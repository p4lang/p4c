extern bool ready();
extern bit<8> identity(in bit<8> value);
extern void consume(in bit<8> value);
control C() {
    @name("C.value") bit<8> value_0;
    @name("C.tmp") bool tmp;
    apply {
        value_0 = 8w1;
        tmp = ready();
        if (tmp) {
            value_0 = identity(value_0);
            consume(value_0);
        }
    }
}

control CT();
package Test(CT c);
Test(C()) main;
