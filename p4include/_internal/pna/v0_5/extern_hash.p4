// BEGIN:Hash_algorithms
enum PNA_HashAlgorithm_t {
  // TBD what this type's values will be for PNA
  TARGET_DEFAULT      /// target implementation defined
}
// END:Hash_algorithms

// BEGIN:Hash_extern
extern Hash<O> {
  /// Constructor
  Hash(PNA_HashAlgorithm_t algo);

  /// Compute the hash for data.
  /// @param data The data over which to calculate the hash.
  /// @return The hash value.
  O get_hash<D>(in D data);

  /// Compute the hash for data, with modulo by max, then add base.
  /// @param base Minimum return value.
  /// @param data The data over which to calculate the hash.
  /// @param max The hash value is divided by max to get modulo.
  ///        An implementation may limit the largest value supported,
  ///        e.g. to a value like 32, or 256, and may also only
  ///        support powers of 2 for this value.  P4 developers should
  ///        limit their choice to such values if they wish to
  ///        maximize portability.
  /// @return (base + (h % max)) where h is the hash value.
  O get_hash<T, D>(in T base, in D data, in T max);
}
// END:Hash_extern
