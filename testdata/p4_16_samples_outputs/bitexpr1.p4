const bit<2 + 2> a = 5;
const bit<(a < 3 ? 5 : 4)> b = 6;
extern T func<T>(T arg);
action act(bit<5> x) {
    func<bit<5>>(x);
    func<bit<b ^ 3>>(x);
}
