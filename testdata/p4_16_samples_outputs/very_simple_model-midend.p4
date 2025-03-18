#include <core.p4>

struct InControl {
    bit<4> inputPort;
}

struct OutControl {
    bit<4> outputPort;
}

extern Ck16 {
    Ck16();
    void clear();
    void update<T>(in T data);
    bit<16> get();
}

