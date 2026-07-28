extern PlainExtern {
  PlainExtern(bit<16> arg);
}

extern ParametrizedExtern<ArgT> {
  ParametrizedExtern(ArgT arg);
}

extern OtherExtern {
  OtherExtern();
  void apply_any<CatchAllT>(CatchAllT arg);
}

parser Parser() {
  PlainExtern(16w1) plain;
  ParametrizedExtern(16w1) parametrized;
  OtherExtern() ctrl;
  
  state start {
    ctrl.apply_any(plain); // OK
    ctrl.apply_any(parametrized); // Not OK

    transition accept;
  }
}
