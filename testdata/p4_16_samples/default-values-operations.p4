#include <core.p4>

header H {
    bit<1>  a;
    bool    b;
    bit<32> c;
}

struct S1 {
    bit<1> i;
    H      h;
    bool   b;
}

struct S {
    bit<32> a;
    bit<32> b;
}

S1 f(in S1 data) {
    bit<1> i = data.i;
    return {i = i, ... };
}
control c(inout S1 r) {
    apply {
        S1 val = f({i = 1, ... });
        r.i = val.i;
        S s = {a = 11,b = 22};
        bool b1 = (S){a = 11,b = 11} == {a = 1,b = 2};
        bool b1_ = (S){a = 11, ... } == {a = 1,b = 1};
        bool b1__ = {a = 1,b = 1} == (S){a = 11, ... };
        bool b2 = s == {a = 1,b = 2};
        bool b2_ = {a = 11, ... } == s;
        bool b2__ = s == {a = 11, ... };
        bool b3 = s == (S){a = 1,b = 2};
        bool b3_ = s == (S){a = 1, ... };
        bool b3__ = (S){a = 1, ... } == s;
        bool b4 = {a = 11,b = 22} == (S){ 1, 2 };
        bool b4_ = {a = 11,b = 22} == (S){ 1, ... };
        bool b4__ = (S){ 1, ... } == {a = 11,b = 22};
        tuple<bit<32>, bit<32>> t = { 1, 2 };
        bool b5 = t == { 1, 2 };
        bool b5_ = t == { 1, ... };
        bool b5__ = { 1, ... } == t;
    }
}

control simple(inout S1 r);
package top(simple e);
top(c()) main;

