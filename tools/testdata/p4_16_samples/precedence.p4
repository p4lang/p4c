/*
Copyright 2013-present Barefoot Networks, Inc. 

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

struct s { bit z; bit w; }
struct t { s s1; s s2; }
typedef bit Bit;
extern bit fct(in bit a, in bit b);

action ac()
{
    bit a;
    bit b;
    bit c;
    bit d;
    bool e;
    s f;
    t g;

    a = b + c + d;
    a = (b + c) + d;
    a = b + (c + d);
    a = b * c + d;
    a = b * (c + d);
    a = b + c * d;
    a = (b + c) * d;
    a = b * c * d;
    a = b * (c * d);
    a = b + c / d;
    a = (b + c) / d;
    a = b / c + d;
    a = b / (c + d);
    a = b / c / d;
    a = b / (c / d);
    a = b | c | d;
    a = (b | c) | d;
    a = b | (c | d);
    a = e ? c : d;
    a = e ? c + c : d + d;
    a = e ? (c + c) : (d + d);
    a = d + (e ? (c + c) : d);
    a = (e ? (c + c) : d) + d;
    a = e ? (e ? b : c) : (e ? b : c);
    a = (e ? (e ? b : c) : b) == b ? b : c;
    a = b & c | d;
    a = b | c & d;
    a = (b & c) | d;
    a = b & (c | d);
    a = (b | c) & d;
    a = b | (c & d);
    e = e && e || e;
    e = e || e && e;
    e = (e && e) || e;
    e = e && (e || e);
    e = (e || e) && e;
    e = e == e == e;
    e = (e == e) == e;
    e = e == (e == e);
    e = e != e != e;
    e = (e != e) != e;
    e = e != (e != e);
    e = e == e != e;
    e = (e == e) != e;
    e = e == (e != e);
    e = e != e == e;
    e = (e != e) == e;
    e = e != (e == e);
    e = a < b == e;
    e = e == a < b;
    e = (a < b) == e;
    a = a << b << c;
    a = a << (b << c);
    a = a << b >> c;
    a = a << (b >> c);
    a = f.z + a;
    a = fct(a, b);
    a = fct(a+b, a+c);
    a = fct(a, fct(b, a) + c);
    a = fct(a+(b*c), a*(b+c));
    a = (Bit)b;
    a = (Bit)b + c;
    a = (Bit)(b + c);
    a = (Bit)f.z + b;
    a = (Bit)fct((Bit)(f.z) + b, b+c);

    f = { a+b, c };
    g = { {a+b, c}, {a+b, c} };
    g = { ({ (Bit)(a+b)+b, c}), {a+(b+c), c} };
}
