struct s {}

header h
{
    s field;  // no struct fields allowed in headers
}

parser p();

struct s1
{
    p field;  // no functor-typed fields allowed
}

header_union u
{
   s field;   // no struct field allowed in header_union
   bit field1;  // no non-header field allowed in header_union
}
