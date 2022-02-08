#include <core.p4>


struct S {
    bit<32> a;
    bit<32> b;
}

control compute(bit<1> r) {
    apply {
	 S s = {a = 11, b = 22};
         bool b1 = {a = 1, b = 1} == {a = 11, b = 22}; // error: ==: cannot compare initializers with unknown types
         bool _b5 = {1, ...} == {2, ...}; // error: Operation relation == cannot be used in a case where both operands are default initialized 
       
    }
}

control simple(bit<1> r);
package top(simple e);
top(compute()) main;
