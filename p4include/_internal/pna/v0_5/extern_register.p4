// BEGIN:Register_extern
extern Register<T, S> {
  /// Instantiate an array of <size> registers. The initial value is
  /// undefined.
  Register(bit<32> size);
  /// Initialize an array of <size> registers and set their value to
  /// initial_value.
  Register(bit<32> size, T initial_value);

  T    read  (in S index);
  void write (in S index, in T value);
}
// END:Register_extern
