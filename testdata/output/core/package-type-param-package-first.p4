package Inner();
package Outer<P>(P p);
Outer<Inner>(Inner()) outer_explicit;
Outer<Inner>(Inner()) outer_inferred;
package Top(Outer<Inner> a, Outer<Inner> b);
Top(outer_explicit, outer_inferred) main;
