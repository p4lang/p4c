// tests problem solved by https://github.com/p4lang/p4c/pull/4411

#include <core.p4>

@foo[bar=4<H]
control p<H>(in H hdrs, out bool flag)
{
    apply {
    }
}
