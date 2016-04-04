#include "core.p4"

control c(in bit x)
{
    table t()
    {
        key = { x : exact; }
    }

    apply {}
}
