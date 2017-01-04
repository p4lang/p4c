@annotest const bit<1> b = 1w1;
extern Annotated {
    Annotated();
}

extern bit<32> log(in bit<32> data);
control c() {
    apply {
        @blockAnnotation {
        }
    }
}

