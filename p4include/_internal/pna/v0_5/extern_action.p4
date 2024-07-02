// BEGIN:ActionProfile_extern
extern ActionProfile {
  /// Construct an action profile of 'size' entries
  ActionProfile(bit<32> size);
}
// END:ActionProfile_extern

// BEGIN:ActionSelector_extern
extern ActionSelector {
  /// Construct an action selector of 'size' entries
  /// @param algo hash algorithm to select a member in a group
  /// @param size number of entries in the action selector
  /// @param outputWidth size of the key
  ActionSelector(PNA_HashAlgorithm_t algo, bit<32> size, bit<32> outputWidth);
}
// END:ActionSelector_extern
