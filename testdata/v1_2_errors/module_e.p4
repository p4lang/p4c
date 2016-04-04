parser Filter(out bool filter);

package top(Filter f);

parser g(in bit x) // mismatch in direction
{
    state start {}
}

top(g()) main;
