bit<1> func(bit<1> t1) {
    tuple<int> t = { t1 };
    return (bit<1>)t[0];
}
bit<1> func1(bit<1> t1) {
    tuple<int> t = { t1 };
    return (bit<1>)t[0];
}
bit<1> func2(bit<1> t1) {
    tuple<int> t = { 1 };
    return (bit<1>)t[0];
}
