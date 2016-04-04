control p()
{
    apply {}
}

control q()
{
    apply {
        p.apply();
    }
}
