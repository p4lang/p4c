extern void f(out bit<32> x = 0,   // illegal: out parameter with default value
             @optional bit<32> y = 0); // illegal: optional parameter with default value
