extern E<T, S> {
    E(tuple<T, S> data);
}

control c0() {
    E<bit<32>, bit<16>>((tuple<bit<32>, varbit<16>>){ 2, 3 }) e;
    apply {
    }
}

