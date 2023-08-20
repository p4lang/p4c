@size(100) extern Annotated {
    @name("annotated") Annotated();
    @name("exe") void execute(bit<8> index);
}

@cycles(10) extern bit<32> log(in bit<32> data);
