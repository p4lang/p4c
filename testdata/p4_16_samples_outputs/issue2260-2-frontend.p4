control C();
package S(C c);
control MyC() {
    apply {
        {
            bool hasReturned = false;
            bit<8> retval;
            hasReturned = true;
            retval = 8w255;
        }
    }
}

S(MyC()) main;

