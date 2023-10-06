parser p() {
    state start {
        transition accept;
    }
}

void func()
{
    p() t;
    t();
}