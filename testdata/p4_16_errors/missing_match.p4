control c(in bit b)
{
    action NoAction() {}
    
    table t() {
        key = { b : noSuchMatch; }
        actions = { NoAction; }
    }

    apply {}
}
