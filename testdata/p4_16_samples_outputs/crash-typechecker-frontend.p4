header H {
    bit<8>      len;
    @length(((bit<32>)len << 3) - 32w16) 
    varbit<304> data;
}

