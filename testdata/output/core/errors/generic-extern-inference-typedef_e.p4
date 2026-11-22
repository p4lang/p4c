extern Parametrized<ArgT> {
    Parametrized(ArgT arg);
    ArgT get();
}

typedef Parametrized<bit<16>> Parametrized16;
control c() {
    Parametrized16<bit<16>>(16w1) alreadySpecialized;
    apply {
    }
}

