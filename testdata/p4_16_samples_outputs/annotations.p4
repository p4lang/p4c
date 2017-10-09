@annotest const bit<1> b = 1;
extern Annotated {
    Annotated();
    void execute(bit<8> index);
}

extern bit<32> log(in bit<32> data);
control c() {
    apply {
        @blockAnnotation {
        }
    }
}

