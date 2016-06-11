extern If<T>
{
    T id(in T d);
}

control p1(in If x) // missing type parameter
{
    apply {}
}

control p2(in If<int<32>, int<32>> x) // too many type parameters
{
    apply {}
}

header h {}

control p()
{
    apply {
        h<bit> x;     // no type parameter
    }
}
