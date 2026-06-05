const bit<4> a = 4w5;
const bit<4> b = 4w6;
extern T func<T>(T arg);
action act(bit<5> x) {
    func<bit<5>>(x);
    func<bit<5>>(x);
}
