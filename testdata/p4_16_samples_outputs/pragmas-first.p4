@annotest const bit<1> b = 1w1;
@size(100) extern Annotated {
    @name("annotated") Annotated();
    @name("exe") void execute(bit<8> index);
}

@cycles(10) extern bit<32> log(in bit<32> data);
control c() {
    apply {
        @blockAnnotation {
        }
    }
}

