control C<H>(inout H h);
package Pipeline<H>(C<H> c);
package Switch<H0, H1>(Pipeline<H0> p0, @optional Pipeline<H1> p1);


struct header_t {
}
control MyC(inout header_t h) {
    apply {}
}

Pipeline(MyC()) pipe;
Switch(pipe) main;
