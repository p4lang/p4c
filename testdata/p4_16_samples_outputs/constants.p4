control p() {
    apply {
        bit<1> x;
        bit<1> z;
        const bit<1> c = 1w0;
        if (true) 
            x = 1w0;
        else 
            x = 1w1;
        if (false) 
            z = 1w0;
        else 
            if (true && false) 
                z = 1w1;
        if (c == 1w0) 
            z = 1w0;
    }
}

