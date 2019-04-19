header H {
    bit<8>      len;
    @length(((bit<32>)len << 3) + 32w4294967280) 
    varbit<304> data;
}

