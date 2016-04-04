header Hdr {
    varbit<256> data0;
    @length(data0) 
    varbit<12>  data1;
    @length(size2) 
    varbit<256> data2;
    int<32>     size2;
}

