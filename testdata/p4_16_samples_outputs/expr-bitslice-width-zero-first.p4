const bool tmp1 = true;
const bool tmp2 = true;
control c(inout bit<8> x) {
    apply {
        x = x | 8w1;
    }
}

