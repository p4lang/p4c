header H {
}

control C() {
    H[2] stack;
    apply {
        stack = (list<match_kind>){ (H){#}, ... };
    }
}

