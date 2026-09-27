extern bool ready();
extern bit<8> identity(in bit<8> value);
extern void consume(in bit<8> value);
control C() {
    @name("C.value") bit<8> value_0;
    @name("C.tmp") bool tmp;
    @hidden action refcounttypelifetime17() {
        value_0 = identity(8w1);
        consume(value_0);
    }
    @hidden action refcounttypelifetime14() {
        value_0 = 8w1;
        tmp = ready();
    }
    @hidden table tbl_refcounttypelifetime14 {
        actions = {
            refcounttypelifetime14();
        }
        const default_action = refcounttypelifetime14();
    }
    @hidden table tbl_refcounttypelifetime17 {
        actions = {
            refcounttypelifetime17();
        }
        const default_action = refcounttypelifetime17();
    }
    apply {
        tbl_refcounttypelifetime14.apply();
        if (tmp) {
            tbl_refcounttypelifetime17.apply();
        }
    }
}

control CT();
package Test(CT c);
Test(C()) main;
