// A typedef of a specialized generic extern type is not itself generic, so it cannot be given
// type arguments.  This is the reason why inferred type arguments are only written out for names
// which refer to the generic declaration itself.

extern Parametrized<ArgT> {
    Parametrized(ArgT arg);
    ArgT get();
}

typedef Parametrized<bit<16>> Parametrized16;

control c() {
    Parametrized16<bit<16>>(16w1) alreadySpecialized;
    apply {}
}
