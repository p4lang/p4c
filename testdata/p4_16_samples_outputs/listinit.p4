extern test<T1, T2> {
    test(list<tuple<T1, T2>> pairs);
}

test<bit<16>, _>({ { 16w0, "foo" }, { 16w1, "bar" }, { 16w2, "baz" } }) obj;
test<bit<16>, _>({  }) o2;
test<bit<16>, bit<8>>({ { 0, 10 }, { 1, 12 }, { 2, 15 } }) o3;
