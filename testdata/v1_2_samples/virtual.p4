// In arch file
extern Virtual {
    // abstract methods must be implemented
    // by the users
    abstract bit<16> f(in bit<16> ix);
}

// User code
control c(inout bit<16> p) {
    Virtual() cntr = {  // implementation
        bit<16> f(in bit<16> ix) {  // abstract method implementation
            return (ix + 1);
        }
    };

    apply {
        // abstract methods are not necessarily called by
        // user code (as here); they may be called by the extern
        // internally as part of its computation.
        p = cntr.f(6);
    }
}

control ctr(inout bit<16> x);
package top(ctr ctrl);

top(c()) main;
