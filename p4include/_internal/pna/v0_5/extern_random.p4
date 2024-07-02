// BEGIN:Random_extern
extern Random<T> {

  /// Return a random value in the range [min, max], inclusive.
  /// Implementations are allowed to support only ranges where (max -
  /// min + 1) is a power of 2.  P4 developers should limit their
  /// arguments to such values if they wish to maximize portability.

  Random(T min, T max);
  T read();
}
// END:Random_extern
