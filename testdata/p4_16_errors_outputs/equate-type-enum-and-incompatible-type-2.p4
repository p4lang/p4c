enum X {
    Field
}

control c() {
    apply {
        tuple<> v = X.Field;
    }
}

