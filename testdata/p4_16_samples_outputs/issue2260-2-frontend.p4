control C();
package S(C c);
control MyC() {
    apply {
        {
            @name("MyC.hasReturned") bool hasReturned = false;
            @name("MyC.retval") bit<8> retval;
            hasReturned = true;
            retval = 8w255;
        }
    }
}

S(MyC()) main;

