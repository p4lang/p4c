control C();
package S(C c);
control MyC() {
    @name("MyC.hasReturned") bool hasReturned;
    @name("MyC.retval") bit<8> retval;
    apply {
        hasReturned = false;
        hasReturned = true;
        retval = 8w255;
    }
}

S(MyC()) main;

