header Hdr {
    varbit<256> data0;
    
    @length(data0)      // illegal: expression is not uint<32>
    varbit<12> data1;
    
    @length(size2)     // illegal: cannot use size2, defined after data2
    varbit<256> data2;
    
    int<32> size2;    
}
