// tests problems related to https://github.com/p4lang/p4c/pull/4411

#include <core.p4>

control ctrl<H>(bool val);

package pkg<H>(
    ctrl<H> val = ctrl(H > 3)
);
