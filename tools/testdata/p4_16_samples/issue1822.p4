control C<X>();
package S<X>(C<X> x1, C<X> x2);
control MyC()() { apply {} }
S<bool>(MyC(), MyC()) main;
