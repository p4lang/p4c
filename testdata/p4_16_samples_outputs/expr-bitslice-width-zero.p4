const bool tmp1 = 8w2[0+:0] == 0;
const bool tmp2 = 8w2[3+:0] == 0;
control c(inout bit<8> x) {
    apply {
        if (x[2+:0] == 0) {
            x = x | 1;
        }
    }
}

